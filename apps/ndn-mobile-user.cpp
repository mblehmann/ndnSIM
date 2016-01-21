/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Matheus Lehmann <mblehmann@inf.ufrgs.br>
 */

#include "ndn-mobile-user.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/pointer.h"

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"
 
#include <list>

using namespace std;

NS_LOG_COMPONENT_DEFINE("ndn.MobileUser");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(MobileUser);

TypeId
MobileUser::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::MobileUser")
      .SetGroupName("Ndn")
      .SetParent<Consumer>()
      .AddConstructor<MobileUser>()
      .AddAttribute("WindowSize", "Interest window size", IntegerValue(0),
                    MakeIntegerAccessor(&MobileUser::m_windowSize), MakeIntegerChecker<int32_t>())
      .AddAttribute("NameService", "Name service to discover content", PointerValue(NULL), MakePointerAccessor(&MobileUser::m_nameService),
                   MakePointerChecker<NameService>());
  return tid;
}

MobileUser::MobileUser()
  : m_windowSize(1)
  , m_pendingInterests(0)
  , m_chunksRetrieved(0)
{
  NS_LOG_FUNCTION_NOARGS();

  m_rtt = CreateObject<RttMeanDeviation>();
}

// Application Methods
void
MobileUser::StartApplication() // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS();

  // do base stuff
  App::StartApplication();

  NS_LOG_INFO("> Waiting to discover an object");
}

void
MobileUser::StopApplication() // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS();

  // cancel periodic packet generation
  Simulator::Cancel(m_sendEvent);

  // cleanup base stuff
  App::StopApplication();
}

void
MobileUser::SetRetxTimer(Time retxTimer)
{
  m_retxTimer = retxTimer;
  if (m_retxEvent.IsRunning()) {
    // m_retxEvent.Cancel (); // cancel any scheduled cleanup events
    Simulator::Remove(m_retxEvent); // slower, but better for memory
  }

  // schedule even with new timeout
  m_retxEvent = Simulator::Schedule(m_retxTimer, &MobileUser::CheckRetxTimeout, this);
}

void
MobileUser::CheckRetxTimeout()
{
  Time now = Simulator::Now();

  Time rto = m_rtt->RetransmitTimeout();
  
  Name entryName;
  Time entryTime;

  while (!m_nameTimeouts.empty()) {
    entryName = m_nameTimeouts.begin()->first;
    entryTime = m_nameTimeouts.begin()->second;
    
    for (map<Name, Time>::iterator it=m_nameTimeouts.begin(); it!=m_nameTimeouts.end(); ++it)
    {
      if (it->second < entryTime)
      {
        entryName = it->first;
        entryTime = it->second;
      }   
    }

    if (entryTime + rto <= now)
    {
      Name expiredInterest = entryName;
      m_nameTimeouts.erase(entryName);
      OnTimeout(expiredInterest);
    }
    else
      break; // nothing else to do. All later packets need not be retransmitted
  }

  m_retxEvent = Simulator::Schedule(m_retxTimer, &MobileUser::CheckRetxTimeout, this);
}


/**
 * Receives the data.
 * Removes the interest packet on the queue.
 * Checks if it is the last chunk of the object (for measurement purpose).
 * Schedules next packet to request.
 */
void
MobileUser::OnData(shared_ptr<const Data> data)
{

  // // Do the regular processing
  App::OnData(data); // tracing inside
  NS_LOG_FUNCTION(this << data);

  //Consumer::OnData(data);
  Name objectName = data->getName();
  NS_LOG_INFO("> Data for " << objectName);

  // Reduce the number of pending interest requests
  m_pendingObjects.remove(objectName);
  m_pendingInterests--;

  // Check if it is the last chunk
  Name objectPrefix = objectName.getSubName(0, 2);
  Name pendingPrefix;
  bool concluded = true;

  for (uint32_t i = 0; i < m_interestQueue.size() && concluded; i++) {
    pendingPrefix = m_interestQueue[i].getSubName(0, 2);
    if (objectPrefix == pendingPrefix) {
      concluded = false;
      break;
    } 
  }

  for (list<Name>::iterator it = m_pendingObjects.begin(); it != m_pendingObjects.end() && concluded; ++it) {
    pendingPrefix = it->getSubName(0, 2);
    if (objectPrefix == pendingPrefix) {
      concluded = false;
      break;
    } 
  }

  for (uint32_t i = 0; i < m_retxNames.size() && concluded; i++) {
    pendingPrefix = m_retxNames[i].getSubName(0, 2);
    if (objectPrefix == pendingPrefix) {
      concluded = false;
      break;
    } 
  }

  if (concluded)
    ConcludeObjectDownload(objectPrefix);

  int hopCount = 0;
  auto ns3PacketTag = data->getTag<Ns3PacketTag>();
  if (ns3PacketTag != nullptr) { // e.g., packet came from local node's cache
    FwHopCountTag hopCountTag;
    if (ns3PacketTag->getPacket()->PeekPacketTag(hopCountTag)) {
      hopCount = hopCountTag.Get();
      NS_LOG_DEBUG("Hop count: " << hopCount);
    }
  }

  Time lastDelay = m_nameLastDelay[objectName];
  m_lastRetransmittedInterestDataDelay(this, objectName, Simulator::Now() - lastDelay, hopCount);

  Time fullDelay = m_nameFullDelay[objectName];
  m_firstInterestDataDelay(this, objectName, Simulator::Now() - fullDelay, m_nameRetxCounts[objectName], hopCount);

  m_nameRetxCounts.erase(objectName);
  m_nameFullDelay.erase(objectName);
  m_nameLastDelay.erase(objectName);

  m_nameTimeouts.erase(objectName);

  m_rtt->AckSeq(m_chunkOrder[objectName]);

  // Schedules next packets
  ScheduleNextPacket();
}

/**
 * 
 */
void
MobileUser::OnTimeout(Name objectName)
{
  NS_LOG_FUNCTION(objectName);
  std::cout << Simulator::Now () << ", TO: " << objectName << ", current RTO: " <<
   m_rtt->RetransmitTimeout ().ToDouble (Time::S) << "s\n";

  m_rtt->IncreaseMultiplier(); // Double the next RTO
  m_rtt->SentSeq(m_chunkOrder[objectName],
                 1); // make sure to disable RTT calculation for this sample
  m_retxNames.push_back(objectName);
  
  m_pendingObjects.remove(objectName);
  m_pendingInterests--;

  ScheduleNextPacket();
}

/**
 * 
 */
void
MobileUser::SendPacket()
{
  Name object;

  if (!m_active) 
    return;

  if (m_retxNames.size() > 0) {
    object = m_retxNames.front();
    m_retxNames.erase(m_retxNames.begin());
  }
  else if (m_interestQueue.size() > 0) {
    object = m_interestQueue.front();
    m_interestQueue.erase(m_interestQueue.begin());
  }
  else {
    return;
  }

  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(object);
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  // NS_LOG_INFO ("Requesting Interest: \n" << *interest);
  NS_LOG_INFO("> Interest for " << interest->getName());

  WillSendOutInterest(object);

  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);

  m_pendingObjects.push_back(interest->getName());
  m_pendingInterests++;
}

/**
 * 
 */
void
MobileUser::WillSendOutInterest(Name objectName)
{
  NS_LOG_DEBUG("Trying to add " << objectName << " with " << Simulator::Now() << ". already "
                                << m_nameTimeouts.size() << " items");

  m_nameTimeouts[objectName] = Simulator::Now();
  m_nameFullDelay[objectName] = Simulator::Now();

  m_nameLastDelay.erase(objectName);
  m_nameLastDelay[objectName] = Simulator::Now();

  m_nameRetxCounts[objectName]++;

  m_rtt->SentSeq(m_chunkOrder[objectName], 1);
}

/**
 * 
 */
void
MobileUser::ScheduleNextPacket()
{
  if (m_interestQueue.size() > 0) {
    NS_LOG_INFO("> Scheduling next packet");

    while (m_pendingInterests < m_windowSize) {
      //Simulator::ScheduleNow(&Consumer::SendPacket, this);
      //Simulator::ScheduleNow(&MobileUser::SendPacket, this);
      SendPacket();
    }
  }

}

/**
 * Adds a new object to the interest queue. It creates the name of the interest requests and puts it into the interest queue.
 */
void
MobileUser::AddInterestObject(Name objectName, uint32_t chunks)
{
  NS_LOG_INFO("> Adding Interest for object " << objectName << "/" << chunks);

  m_seqMax = 7;
  //create the interest name
  shared_ptr<Name> interestName;

  //for each chunk, append the sequence number and add it to the interest queue
  for (uint32_t i = 0; i < chunks; i++) {
    interestName = make_shared<Name>(objectName);
    interestName->appendSequenceNumber(i);
    m_interestQueue.push_back(*interestName);
  }
  
  NS_LOG_INFO("> Chunk " << m_interestQueue.back());
  NS_LOG_INFO("> Chunk " << m_interestQueue.front());

  Simulator::Schedule(Time("2s"), &MobileUser::ScheduleNextPacket, this);

}

/**
 * Get performance measurements of the download
 */
void
MobileUser::ConcludeObjectDownload(Name objectName)
{
  NS_LOG_INFO("Finished Dowlonading:" << objectName);

  for (uint32_t i = 0; i < m_providedObjects.size(); i++) {
    if (objectName == m_providedObjects[i]) {
      AnnounceContent(objectName);
      break;
    }
  }
}

void
MobileUser::AnnounceContent(Name object)
{
  // Create the announcement packet
  shared_ptr<Announcement> announcement = make_shared<Announcement>(object);
  NS_LOG_INFO("> Creating the announcement for object " << object);

  NS_LOG_INFO("> Generating the Nonce " << m_rand->GetValue(0, numeric_limits<uint32_t>::max()));
  announcement->setNonce(m_rand->GetValue(0, numeric_limits<uint32_t>::max()));

  // Log information
  NS_LOG_INFO("> Announcement for " << object);

  // Send the packet
  announcement->wireEncode();

  m_transmittedAnnouncements(announcement, this, m_face);
  m_face->onReceiveAnnouncement(*announcement);
}

void
MobileUser::OnAnnouncement(shared_ptr<const Announcement> announcement)
{
  App::OnAnnouncement(announcement); 

  NS_LOG_INFO("> LE ANNOUNCEMENT");
}

void
MobileUser::OnHint(shared_ptr<const Hint> hint)
{
  App::OnHint(hint);

  Name objectName = hint->getName();
  int nodeID = hint->getNodeID();

  NS_LOG_INFO("> Numbers: " << nodeID << " " << this->GetId());

  if (nodeID == this->GetId()) {
    NS_LOG_INFO("> Received hint for object " << objectName);

    uint32_t chunks = 7;

    AddInterestObject(objectName, chunks);
    m_providedObjects.push_back(objectName);
  }

}

void
MobileUser::OnVicinity(shared_ptr<const Vicinity> vicinity)
{
  App::OnVicinity(vicinity);

  Name objectName = vicinity->getName();
  NS_LOG_INFO("> Received vicinity probe for object " << vicinity << " " << objectName);

  // Create the vicinity packet
  shared_ptr<VicinityData> vicinityData = make_shared<VicinityData>();
  vicinityData->setName(objectName);
  vicinityData->setNodeID(this->GetId());

  // Log information
  NS_LOG_INFO("> Vicinity data response for " << objectName);

  // Send the packet
  vicinityData->wireEncode();

  m_transmittedVicinityDatas(vicinityData, this, m_face);
  m_face->onReceiveVicinityData(*vicinityData);

}

void
MobileUser::OnVicinityData(shared_ptr<const VicinityData> vicinityData)
{
  App::OnVicinityData(vicinityData);

  NS_LOG_INFO("> LE VICINITY DATA");
}

} // namespace ndn
} // namespace ns3


