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
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "ns3/random-variable-stream.h"

#include "utils/ndn-ns3-packet-tag.hpp"
#include "model/ndn-app-face.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include "helper/ndn-fib-helper.hpp"

#include "stdlib.h"
#include "time.h"

#include <boost/lexical_cast.hpp>
#include <boost/ref.hpp>

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
      .SetParent<App>()
      .AddConstructor<MobileUser>()

      .AddAttribute("WindowSize", "Interest window size",
                    IntegerValue(3),
                    MakeIntegerAccessor(&MobileUser::m_windowSize),
                    MakeIntegerChecker<int32_t>())

      .AddAttribute("ReplicationDegree", "Number of replicas pushed",
                    UintegerValue(1),
                    MakeUintegerAccessor(&MobileUser::m_replicationDegree),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("VicinityTimer", "Period to discover vicinity",
                    StringValue("10s"),
                    MakeTimeAccessor(&MobileUser::m_vicinityTimer),
                    MakeTimeChecker())

      .AddAttribute("NameService", "Name service to discover content",
                    PointerValue(NULL),
                    MakePointerAccessor(&MobileUser::m_nameService),
                    MakePointerChecker<NameService>())

      .AddAttribute("LifeTime", "LifeTime for interest packet",
                    StringValue("2s"),
                    MakeTimeAccessor(&MobileUser::m_interestLifeTime),
                    MakeTimeChecker())

      .AddAttribute("RetxTimer",
                    "Timeout defining how frequent retransmission timeouts should be checked",
                    StringValue("50ms"),
                    MakeTimeAccessor(&MobileUser::GetRetxTimer, &MobileUser::SetRetxTimer),
                    MakeTimeChecker())

      .AddAttribute("Prefix", "Prefix, for which producer has the data",
                    StringValue("/"),
                    MakeNameAccessor(&MobileUser::m_prefix),
                    MakeNameChecker())
      
      .AddAttribute("Postfix", "Postfix that is added to the output data (e.g., for adding producer-uniqueness)",
                    StringValue("/"),
                    MakeNameAccessor(&MobileUser::m_postfix),
                    MakeNameChecker())
      
      .AddAttribute("PayloadSize", "Virtual payload size for Content packets",
                    UintegerValue(1024),
                    MakeUintegerAccessor(&MobileUser::m_virtualPayloadSize),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                    TimeValue(Seconds(0)),
                    MakeTimeAccessor(&MobileUser::m_freshness),
                    MakeTimeChecker())

      .AddTraceSource("LastRetransmittedInterestDataDelay",
                      "Delay between last retransmitted Interest and received Data",
                      MakeTraceSourceAccessor(&MobileUser::m_lastRetransmittedInterestDataDelay),
                      "ns3::ndn::MobileUser::LastRetransmittedInterestDataDelayCallback")

      .AddTraceSource("FirstInterestDataDelay",
                      "Delay between first transmitted Interest and received Data",
                      MakeTraceSourceAccessor(&MobileUser::m_firstInterestDataDelay),
                      "ns3::ndn::MobileUser::FirstInterestDataDelayCallback");
        return tid;
}

MobileUser::MobileUser()
  : m_rand(CreateObject<UniformRandomVariable>())
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
  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);

  NS_LOG_INFO("> Registering " << m_prefix << m_postfix);

  Simulator::Schedule(Time("1s"), &MobileUser::PublishContent, this);
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

Time
MobileUser::GetRetxTimer() const
{
  return m_retxTimer;
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

void
MobileUser::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // tracing inside

  NS_LOG_FUNCTION(this << interest);

  if (!m_active)
    return;

  Name dataName(interest->getName());
  // dataName.append(m_postfix);
  // dataName.appendVersion();

  auto data = make_shared<Data>();
  data->setName(dataName);
  data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

  data->setContent(make_shared< ::ndn::Buffer>(m_virtualPayloadSize));

  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }

  signature.setInfo(signatureInfo);
  signature.setValue(::ndn::nonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));

  data->setSignature(signature);

  NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());

  // to create real wire encoding
  data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_face->onReceiveData(*data);
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
MobileUser::SendInterestPacket()
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

  ScheduleNextPacket();
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
    if (m_pendingInterests < m_windowSize) {
      NS_LOG_INFO("> Scheduling next packet");

      if (m_sendEvent.IsRunning()) {
        Simulator::Remove(m_sendEvent);
      }

      m_sendEvent = Simulator::ScheduleNow(&MobileUser::SendInterestPacket, this);
      //SendInterestPacket();
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
  uint32_t nodeID = hint->getNodeID();

  NS_LOG_INFO("> Numbers: " << nodeID << " " << this->GetNode()->GetId());

  if (nodeID == this->GetNode()->GetId()) {
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
  vicinityData->setNodeID(this->GetNode()->GetId());

  // Log information
  NS_LOG_INFO("> Vicinity data response for " << objectName);

  // Send the packet
  vicinityData->wireEncode();

  m_transmittedVicinityDatas(vicinityData, this, m_face);
  m_face->onReceiveVicinityData(*vicinityData);

}

void
MobileUser::setVicinityTimer(Time vicinityTimer)
{
  m_vicinityTimer = vicinityTimer;
}

void
MobileUser::setReplicationDegree(uint32_t replicationDegree)
{
  m_replicationDegree = replicationDegree;
}

/**
 * Create a new content.
 * Sets it popularity through advertisement.
 */
void
MobileUser::PublishContent()
{
  NS_LOG_INFO("> Publishing a new object");

  Name newObject = createContent();

  // Advertise the new content
  advertiseContent(newObject);

  AnnounceContent(newObject);

  discoverVicinity(newObject);

  m_pushedObjects.push(newObject);

  // scheduling events!!
  Simulator::Schedule(m_vicinityTimer, &MobileUser::PushContent, this);

}

/**
 * Create a new content.
 */
Name
MobileUser::createContent()
{
  // Get the index of the next object
  int objectIndex = m_generatedContent.size();

  // Name is /producer/object<index>
  // Objects have segments /producer/object<index>/#seg
  shared_ptr<Name> newObject = make_shared<Name>(m_prefix);
  newObject->append("obj" + to_string(objectIndex));

  // Add to the list of generated content
  m_generatedContent.push_back(*newObject);

  return *newObject;
}

/**
 * Advertise a new content.
 * Puts it into a shared data structure and set who should request it (popularity of the content).
 * I don't know yet how to do it.
 */
void
MobileUser::advertiseContent(Name newObject)
{
  m_nameService->publishContent(newObject);
  uint32_t popularity = m_nameService->nextContentPopularity();
  uint32_t chunks = 7;

  vector<Ptr<Node>> users = m_nameService->getUsers();
  Ptr<Node> currentUser;
  Ptr<MobileUser> mobileUser;
  for (uint32_t i = 0; i < users.size(); i++) {
    Ptr<Node> currentUser = users[i];

    // TODO
    if (currentUser->GetId() != this->GetNode()->GetId() &&
        (m_rand->GetValue(0, 100)) > popularity) {

      Ptr<MobileUser> mobileUser = DynamicCast<MobileUser> (currentUser->GetApplication(0));
      //Simulator::ScheduleNow(&MobileUser::AddInterestObject, &newObject, 3);
      //Simulator::ScheduleNow(&MobileUser::AddInterestObject, &users[i], &newObject, 3);
      //Simulator::ScheduleNow(&MobileUser::AddInterestObject, dynamic_cast<MobileUser*>(&users[i]->GetApplication(0)), &newObject, 3);
      Simulator::ScheduleWithContext(currentUser->GetId(), Time("0s"), &MobileUser::AddInterestObject, mobileUser, newObject, chunks);
    }
  }

  NS_LOG_INFO("> Advertising object " << newObject << " with popularity " << popularity);
}

/**
 * Announce all the objects
 */
void
MobileUser::AnnounceContent()
{
  uint32_t publishedObjects = m_generatedContent.size();
  for (uint32_t i = 0; i < publishedObjects; i++) {
    AnnounceContent(m_generatedContent[i]);
  }
}

/**
 * Announce a specific object
 */
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

/**
 * Send packets to discover the vicinity
 */
void
MobileUser::discoverVicinity(Name object)
{
  m_vicinity.clear();

  // Create the vicinity packet
  shared_ptr<Vicinity> vicinity = make_shared<Vicinity>(object);
  NS_LOG_INFO("> Discovering the vicinity for object " << object);

  // Log information
  NS_LOG_INFO("> Vicinity discovery for " << object);

  // Send the packet
  vicinity->wireEncode();

  m_transmittedVicinities(vicinity, this, m_face);
  m_face->onReceiveVicinity(*vicinity);
}

void
MobileUser::OnVicinityData(shared_ptr<const VicinityData> vicinityData)
{
  Name objectName = vicinityData->getName();
  int remoteNode = vicinityData->getNodeID();
  NS_LOG_INFO("> Vicinity data response RECEIVED from " << remoteNode << " for " << objectName);

  m_vicinity.push_back(remoteNode);
}

/**
 * Execute the whole push content operation
 */
void
MobileUser::PushContent()
{
  Name objectName = m_pushedObjects.front();
  m_pushedObjects.pop();

  NS_LOG_INFO("> Pushing the object " << objectName);

  if (m_vicinity.size() > 0) {

    NS_LOG_INFO("> We have neighbours " << m_vicinity.size());
    int deviceID;

    for (uint32_t i = 0; i < m_replicationDegree; i++) {
      deviceID = selectDevice();
      NS_LOG_INFO("> Selected device " << deviceID);
      sendContent(deviceID, objectName);
    }
  }
}

int
MobileUser::selectDevice()
{
  int position = m_rand->GetValue(0, m_vicinity.size());
  return m_vicinity[position];
}

void
MobileUser::sendContent(int deviceID, Name objectName)
{
  NS_LOG_INFO("> Sending " << objectName << " to " << deviceID);
  // Create the vicinity packet
  shared_ptr<Hint> hint = make_shared<Hint>(objectName);
  hint->setNodeID(deviceID);
  hint->setScope(2);

  // Log information
  NS_LOG_INFO("> Hint to " << deviceID << " for " << objectName);

  // Send the packet
  hint->wireEncode();

  m_transmittedHints(hint, this, m_face);
  m_face->onReceiveHint(*hint);
}

} // namespace ndn
} // namespace ns3


