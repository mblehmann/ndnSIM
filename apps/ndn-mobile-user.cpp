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
  , m_moving(false)
{
  NS_LOG_FUNCTION_NOARGS();

  m_rtt = CreateObject<RttMeanDeviation>();

}

/**
 * Operations executed at the start of the application.
 * The mobile user will register its prefix in the interface.
 */
void
MobileUser::StartApplication() 
{
  NS_LOG_FUNCTION_NOARGS();

  App::StartApplication();
  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);

  Ptr<MobilityModel> mob = this->GetNode()->GetObject<MobilityModel>();
  mob->TraceConnectWithoutContext("CourseChange", MakeCallback(&MobileUser::CourseChange, this));

  // TODO remove it and put in the simulation file
  if (this->GetNode()->GetId() == 4)
  {
    Simulator::Schedule(Time("1s"), &MobileUser::PublishContent, this);
  }
}

/**
 * Operations executed at the end of the application.
 * The mobile user will cancel any send interest event.
 */
void
MobileUser::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  Simulator::Cancel(m_sendEvent);

  App::StopApplication();
}

// RETRANSMISSION CONTROL

/**
 * Set the retransmission check timer.
 * It is executed once in the beginning of the simulation.
 */
void
MobileUser::SetRetxTimer(Time retxTimer)
{
  m_retxTimer = retxTimer;
  if (m_retxEvent.IsRunning()) {
    Simulator::Remove(m_retxEvent);
  }

  m_retxEvent = Simulator::Schedule(m_retxTimer, &MobileUser::CheckRetxTimeout, this);
}

/**
 * Get the retransmission timer.
 */
Time
MobileUser::GetRetxTimer() const
{
  return m_retxTimer;
}

/**
 * Periodically check the retransmission timeout.
 * Finds the Interest with the closest timeout.
 * Checks if that Interest expired (timeout).
 * If that is not the case, do nothing.
 * Re-schedule the event.
 */
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
      break;
  }

  m_retxEvent = Simulator::Schedule(m_retxTimer, &MobileUser::CheckRetxTimeout, this);
}

// PACKET/EVENT HANDLERS

/**
 * Interest packet handler.
 * Receive the interest packet and respond with a data packet.
 */
void
MobileUser::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest);
  NS_LOG_FUNCTION(this << interest);

  if (!m_active)
    return;

  Name dataName(interest->getName());

  // Create data packet
  auto data = make_shared<Data>();
  data->setName(dataName);
  data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

  data->setContent(make_shared< ::ndn::Buffer>(m_virtualPayloadSize));

  // Add signature
  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }

  signature.setInfo(signatureInfo);
  signature.setValue(::ndn::nonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));

  data->setSignature(signature);

  NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());

  // Send it
  data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_face->onReceiveData(*data);
}

/**
 * Data packet handler.
 * Receives the data.
 * Removes the interest packet on the queue.
 * Checks if it is the last chunk of the object (for measurement purpose).
 * Schedules next packet to request.
 */
void
MobileUser::OnData(shared_ptr<const Data> data)
{
  App::OnData(data);
  NS_LOG_FUNCTION(this << data);

  Name objectName = data->getName();

  // Update the pending interest requests
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

  // Calculate the hop count
  int hopCount = 0;
  auto ns3PacketTag = data->getTag<Ns3PacketTag>();
  if (ns3PacketTag != nullptr) { 
    FwHopCountTag hopCountTag;
    if (ns3PacketTag->getPacket()->PeekPacketTag(hopCountTag)) {
      hopCount = hopCountTag.Get();
      NS_LOG_DEBUG("Hop count: " << hopCount);
    }
  }

  // Update the data control structures
  Time lastDelay = m_nameLastDelay[objectName];
  m_lastRetransmittedInterestDataDelay(this, objectName, Simulator::Now() - lastDelay, hopCount);

  Time fullDelay = m_nameFullDelay[objectName];
  m_firstInterestDataDelay(this, objectName, Simulator::Now() - fullDelay, m_nameRetxCounts[objectName], hopCount);

  m_nameRetxCounts.erase(objectName);
  m_nameFullDelay.erase(objectName);
  m_nameLastDelay.erase(objectName);

  m_nameTimeouts.erase(objectName);

  m_rtt->AckSeq(m_chunkOrder[objectName]);

  // Schedule next packets
  ScheduleNextInterestPacket();
}

/**
 * Announcement packet handler.
 * The application should not receive it.
 * Do nothing.
 */ 
void
MobileUser::OnAnnouncement(shared_ptr<const Announcement> announcement)
{
  App::OnAnnouncement(announcement);
}

/**
 * Hint packet handler.
 * The mobile user checks the hint information
 * and decides whether to request the data or not.
 * Decision factors are: nodeID, interest, storage, etc.
 */
void
MobileUser::OnHint(shared_ptr<const Hint> hint)
{
  App::OnHint(hint);

  Name objectName = hint->getName();
  uint32_t nodeID = hint->getNodeID();
  uint32_t chunks = hint->getSize();

  if (nodeID == this->GetNode()->GetId()) {
    AddInterestObject(objectName, chunks);
    m_providedObjects.push_back(objectName);
  }
}

/**
 * Vicinity packet handler.
 * The mobile user receives the probe
 * and responds with its information
 */
void
MobileUser::OnVicinity(shared_ptr<const Vicinity> vicinity)
{
  App::OnVicinity(vicinity);

  Name objectName = vicinity->getName();

  // Create the vicinity packet
  shared_ptr<VicinityData> vicinityData = make_shared<VicinityData>();
  vicinityData->setName(objectName);
  vicinityData->setNodeID(this->GetNode()->GetId());

  // Send the packet
  vicinityData->wireEncode();

  m_transmittedVicinityDatas(vicinityData, this, m_face);
  m_face->onReceiveVicinityData(*vicinityData);
}

/**
 * VicinityData packet handler.
 * The mobile user collects the probe responses
 * and builds a vicinity knowledge.
 */
void
MobileUser::OnVicinityData(shared_ptr<const VicinityData> vicinityData)
{
  Name objectName = vicinityData->getName();
  int remoteNode = vicinityData->getNodeID();

  m_vicinity.push_back(remoteNode);
}

/**
 * Interest packet timeout handler.
 * Update retransmission and rtt statistics.
 * Update the pending interest data structures.
 * Schedule to send a new Interest.  
 */
void
MobileUser::OnTimeout(Name objectName)
{
  NS_LOG_FUNCTION(objectName);
  std::cout << Simulator::Now () << ", TO: " << objectName << ", current RTO: " <<
               m_rtt->RetransmitTimeout ().ToDouble (Time::S) << "s\n";

  m_rtt->IncreaseMultiplier();
  m_rtt->SentSeq(m_chunkOrder[objectName], 1);
  m_retxNames.push_back(objectName);
  
  m_pendingObjects.remove(objectName);
  m_pendingInterests--;

  ScheduleNextInterestPacket();
}

// CONSUMER ACTIONS

/**
 * Send an Interest Packet.
 * The mobile user decides the next object to request.
 * First, it checks the retransmission queue (already failed).
 * Then, the interest queue (not been sent yet).
 * Send the Interest packet.
 * Update the control data structures.
 * Schedule next packet to send.
 */
void
MobileUser::SendInterestPacket()
{
  Name object;

  if (!m_active) 
    return;

  // Select object to request
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

  // Create interest packet
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(object);
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  // Send packet
  WillSendOutInterest(object);

  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);

  // Pending objects control
  m_pendingObjects.push_back(interest->getName());
  m_pendingInterests++;

  ScheduleNextInterestPacket();
}

/**
 * Update the control data structures of the interest packet. 
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
 * Schedule the next Interest packet.
 * Checks if there are pending requests
 * and if it can send them (request window).
 */
void
MobileUser::ScheduleNextInterestPacket()
{
  if (m_interestQueue.size() > 0 || m_retxNames.size() > 0) {
    if (m_pendingInterests < m_windowSize) {
      if (m_sendEvent.IsRunning()) {
        Simulator::Remove(m_sendEvent);
      }

      m_sendEvent = Simulator::ScheduleNow(&MobileUser::SendInterestPacket, this);
    }
  }
}

/**
 * Adds a new object to the interest queue.
 * Creates the name of each chunk to be requested
 * and puts them into the interest queue.
 */
void
MobileUser::AddInterestObject(Name objectName, uint32_t chunks)
{
  shared_ptr<Name> interestName;

  // For each chunk, append the sequence number and add it to the interest queue
  for (uint32_t i = 0; i < chunks; i++) {
    interestName = make_shared<Name>(objectName);
    interestName->appendSequenceNumber(i);
    m_interestQueue.push_back(*interestName);
  }
  
  Simulator::ScheduleNow(&MobileUser::ScheduleNextInterestPacket, this);
}

/**
 * Conclude the download of an object.
 * An object is concluded when all its chunks are retrieved.
 * Gets performance measurements of the download.
 * If the consumer will become a provider, announce it.
 */
void
MobileUser::ConcludeObjectDownload(Name objectName)
{
  for (uint32_t i = 0; i < m_providedObjects.size(); i++) {
    if (objectName == m_providedObjects[i]) {
      AnnounceContent(objectName);
      break;
    }
  }
}

// PROVIDER ACTIONS

/**
 * Announce all the provided content.
 * The content can be either published
 * or provided (as part of the strategy).
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
 * Announce a specific content object.
 * Creates an announcement packet and sends it.
 */
void
MobileUser::AnnounceContent(Name object)
{
  // Create the announcement packet
  shared_ptr<Announcement> announcement = make_shared<Announcement>(object);
  announcement->setNonce(m_rand->GetValue(0, numeric_limits<uint32_t>::max()));

  // Send the packet
  announcement->wireEncode();

  m_transmittedAnnouncements(announcement, this, m_face);
  m_face->onReceiveAnnouncement(*announcement);
}

// PRODUCER ACTIONS

/**
 * Publishes a new content object in the network.
 * Defines the name as /producer_prefix/object<index>/
 * Advertise the content to other consumers
 * and schedule their requests (based on popularity).
 * Announce the content to the routers.
 * Triggers the pushing strategy by discovering the vicinity
 * and schedules an event to push the object.
 */
void
MobileUser::PublishContent()
{
  Name newObject = CreateContentName();
  uint32_t chunks = 7;

  AdvertiseContent(newObject, chunks);

  AnnounceContent(newObject);

  DiscoverVicinity(newObject);

  Simulator::Schedule(m_vicinityTimer, &MobileUser::PushContent, this, newObject, chunks);

}

/**
 * Create a new content object name.
 * Adds it to the list of generated content.
 * Name is /prefix/postfix<index>
 * Objects have segments /prefixr/postfix<index>/#seg
 */
Name
MobileUser::CreateContentName()
{
  int objectIndex = m_generatedContent.size();

  shared_ptr<Name> newObject = make_shared<Name>(m_prefix);
  newObject->append(m_postfix.toUri() + to_string(objectIndex));

  m_generatedContent.push_back(*newObject);

  return *newObject;
}

/**
 * Advertise a new content to other consumers.
 * Based on the popularity of the content, schedule consumers to request it.
 */
void
MobileUser::AdvertiseContent(Name newObject, uint32_t chunks)
{
  m_nameService->publishContent(newObject);
  uint32_t popularity = m_nameService->nextContentPopularity();
  uint32_t chunks = 7;

  vector<Ptr<Node>> users = m_nameService->getUsers();
  Ptr<Node> currentUser;
  Ptr<MobileUser> mobileUser;

  for (uint32_t i = 0; i < users.size(); i++) {
    Ptr<Node> currentUser = users[i];

    // If not the publisher
    if (currentUser->GetId() != this->GetNode()->GetId()) { //&& (m_rand->GetValue(0, 100)) > popularity) {

      Ptr<MobileUser> mobileUser = DynamicCast<MobileUser> (currentUser->GetApplication(0));
      Simulator::ScheduleWithContext(currentUser->GetId(), Time("0s"), &MobileUser::AddInterestObject, mobileUser, newObject, chunks);
    }
  }

  NS_LOG_INFO("> Advertising object " << newObject << " with popularity " << popularity);
}

// STRATEGY ACTIONS

/**
 * Triggers the vicinity discovery process
 * Sends a vicinity packet with a given scope.
 */
void
MobileUser::DiscoverVicinity(Name object)
{
  m_vicinity.clear();

  // Create the vicinity packet
  shared_ptr<Vicinity> vicinity = make_shared<Vicinity>(object);

  // Send the packet
  vicinity->wireEncode();

  m_transmittedVicinities(vicinity, this, m_face);
  m_face->onReceiveVicinity(*vicinity);
}

/**
 * Push the content to other devices.
 * Select the devices on the learnt vicinity
 * and send them the content.
 */
void
MobileUser::PushContent(Name objectName, uint32_t chunks)
{
  if (m_vicinity.size() > 0) {
    int deviceID;

    for (uint32_t i = 0; i < m_replicationDegree; i++) {
      deviceID = SelectDevice();
      SendContent(deviceID, objectName, chunks);
    }
  }
}

/**
 * Select a device from the vicinity.
 */
int
MobileUser::SelectDevice()
{
  int position = m_rand->GetValue(0, m_vicinity.size());
  return m_vicinity[position];
}

/**
 * Send (push) the content to others.
 * We implement it as a hint instead of unsolicited data.
 */
void
MobileUser::SendContent(int deviceID, Name objectName, uint32_t chunks)
{
  // Create the vicinity packet
  shared_ptr<Hint> hint = make_shared<Hint>(objectName, chunks);
  hint->setNodeID(deviceID);
  hint->setScope(2);

  // Send the packet
  hint->wireEncode();

  m_transmittedHints(hint, this, m_face);
  m_face->onReceiveHint(*hint);
}

// MOBILITY ACTIONS

/**
 *
 */
void
MobileUser::Move(Ptr<const MobilityModel> model)
{
  NS_LOG_FUNCTION_NOARGS();

  m_moving = true;

  Vector pos = model->GetPosition();
  NS_LOG_INFO("Starting movement from (" << pos.x << ", " << pos.y << ") to ");
}

/**
 *
 */
void
MobileUser::Session(Ptr<const MobilityModel> model)
{
  NS_LOG_FUNCTION_NOARGS();

  m_moving = false;
 
  Vector pos = model->GetPosition();
  NS_LOG_INFO("Node is at (" << pos.x << ", " << pos.y << ")"); 

}

void
MobileUser::CourseChange(Ptr<const MobilityModel> model)
{
  NS_LOG_FUNCTION_NOARGS();

  if (m_moving)
    Session(model);

  else
    Move(model);

}

} // namespace ndn
} // namespace ns3


