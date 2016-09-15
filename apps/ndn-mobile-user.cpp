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
#include "helper/ndn-global-routing-helper.hpp"

#include "helper/ndn-link-control-helper.hpp"

#include "ndn-cxx/strategy-selectors.hpp"

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

      // Consumer
      .AddAttribute("InitialWindowSize", "Initial interest window size",
                    UintegerValue(10),
                    MakeUintegerAccessor(&MobileUser::m_initialWindowSize),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("LifeTime", "LifeTime for interest packet",
                    StringValue("2s"),
                    MakeTimeAccessor(&MobileUser::m_interestLifeTime),
                    MakeTimeChecker())

      .AddAttribute("RetxTimer",
                    "Timeout defining how frequent retransmission timeouts should be checked",
                    StringValue("50ms"),
                    MakeTimeAccessor(&MobileUser::GetRetxTimer, &MobileUser::SetRetxTimer),
                    MakeTimeChecker())

      // Strategy
      .AddAttribute("ReplicationDegree", "Number of replicas pushed",
                    UintegerValue(0),
                    MakeUintegerAccessor(&MobileUser::m_replicationDegree),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("VicinitySize", "Vicinity size",
                    UintegerValue(0),
                    MakeUintegerAccessor(&MobileUser::m_vicinitySize),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("VicinityTimer", "Period to discover vicinity",
                    StringValue("2s"),
                    MakeTimeAccessor(&MobileUser::m_vicinityTimer),
                    MakeTimeChecker())

      .AddAttribute("HintTimer", "Lifetime of the hint",
                    StringValue("0.1s"),
                    MakeTimeAccessor(&MobileUser::m_hintTimer),
                    MakeTimeChecker())

      .AddAttribute("CacheSize", "Number of content provided on behalf of others",
                    UintegerValue(100),
                    MakeUintegerAccessor(&MobileUser::m_cacheSize),
                    MakeUintegerChecker<uint32_t>())

      // Global
      .AddAttribute("Catalog", "Content catalog",
                    PointerValue(NULL),
                    MakePointerAccessor(&MobileUser::m_catalog),
                    MakePointerChecker<Catalog>())

      // Producer
      .AddAttribute("PublishTime", "Time when the first content object will be published",
                    StringValue("0min"),
                    MakeTimeAccessor(&MobileUser::m_publishTime),
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

      // Tracing
      .AddTraceSource("LastRetransmittedInterestDataDelay",
                      "Delay between last retransmitted Interest and received Data",
                      MakeTraceSourceAccessor(&MobileUser::m_lastRetransmittedInterestDataDelay),
                      "ns3::ndn::MobileUser::LastRetransmittedInterestDataDelayCallback")

      .AddTraceSource("FirstInterestDataDelay",
                      "Delay between first transmitted Interest and received Data",
                      MakeTraceSourceAccessor(&MobileUser::m_firstInterestDataDelay),
                      "ns3::ndn::MobileUser::FirstInterestDataDelayCallback")

      .AddTraceSource("ServedData",
                      "Data served by the provider",
                      MakeTraceSourceAccessor(&MobileUser::m_servedData),
                      "ns3::ndn::MobileUser::ServedDataCallback");

  return tid;
}

MobileUser::MobileUser()
  : m_rand(CreateObject<UniformRandomVariable>())
{
  m_rtt = CreateObject<RttMeanDeviation>();
}

/**
 * Operations executed at the start of the application.
 * The mobile user will register its prefix in the interface.
 */
void
MobileUser::StartApplication() 
{
  App::StartApplication();
  if (m_prefix == "/prod0")
  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
  FibHelper::AddRoute(GetNode(), "/hint", m_face, 0);
  FibHelper::AddRoute(GetNode(), "/vicinity", m_face, 0);
  
  Ptr<MobilityModel> mob = this->GetNode()->GetObject<MobilityModel>();
  mob->TraceConnectWithoutContext("CourseChange", MakeCallback(&MobileUser::CourseChange, this));

  m_moving = false;
  m_movingInterest = false;
  m_movingPublish = false;
  m_movingPush = false;

  m_windowSize = m_initialWindowSize;

  m_position = mob->GetPosition();
  vector<Ptr<Node>> routers = m_catalog->getRouters();
  ndn::LinkControlHelper::UpLink(this->GetNode(), routers[m_position.x]);

  ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::PrintFIBs();

  m_popularity = m_catalog->getUserPopularity();

  // Set probing model - NOT FINAL SOLUTION
  m_hintProbing = true;
  m_probingModel = "ranked";
  //m_probingModel = "random";

  // Set Availability and Interested attributes for each user
  //m_userAvailability = 1;
  m_userAvailability = (uint32_t) m_rand->GetValue(1,6);
  m_userInterested = true;

  // Set thresholds for vicinity probing - TEMPORARY SOLUTION
  m_availabilityThreshold = 3; 
  m_interestedThreshold = true;
 
  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " initial position at router " << m_position.x << " with popularity " << m_popularity << " and availability " << m_userAvailability );

  if (!m_publishTime.IsZero())
    Simulator::Schedule(m_publishTime, &MobileUser::PublishContent, this);

  Simulator::Schedule(m_catalog->getMaxSimulationTime() - Seconds(0.1), &MobileUser::EndGame, this);
}


/**
 * Operations executed at the end of the application.
 * The mobile user will cancel any send interest event.
 */
void
MobileUser::StopApplication()
{
  Simulator::Cancel(m_sendEvent);
  App::StopApplication();
}

void
MobileUser::EndGame()
{
  vector<Name> missingObjects;
  Name objectName;

  for (uint32_t i = 0; i < m_interestQueue.size(); i++) {
    missingObjects.push_back(m_interestQueue[i]);
  }

  for (list<Name>::iterator it = m_pendingObjects.begin(); it != m_pendingObjects.end(); ++it) {
    missingObjects.push_back(*it);
  }

  for (uint32_t i = 0; i < m_retxNames.size(); i++) {
    missingObjects.push_back(m_retxNames[i]);
  }

  for (uint32_t i = 0; i < missingObjects.size(); i++) {
    objectName = missingObjects[i];

    Time lastDelay = m_nameLastDelay[objectName];
    m_lastRetransmittedInterestDataDelay(this, objectName, Seconds(0), 0);

    Time fullDelay = m_nameFullDelay[objectName];
    m_firstInterestDataDelay(this, objectName, Seconds(0), m_nameRetxCounts[objectName], 0);

    m_nameRetxCounts.erase(objectName);
    m_nameFullDelay.erase(objectName);
    m_nameLastDelay.erase(objectName);

    m_nameTimeouts.erase(objectName);
  }
  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " concluded end game with " << missingObjects.size() << " unfinished objects");

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
//  NS_LOG_INFO ("Current RTO: " << rto.ToDouble (Time::S) << "s");

  Name entryName;
  Time entryTime;

  bool timeout = false;

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

//    NS_LOG_INFO("ENTRY: " << entryTime + rto << " " << now);

    if (entryTime + rto <= now)
    {
      Name expiredInterest = entryName;
      m_nameTimeouts.erase(entryName);
      OnTimeout(expiredInterest);
      timeout = true;
    }
    else
      break;
  }

  if (timeout)
  {
    m_rtt->IncreaseMultiplier();
    ScheduleNextInterestPacket(true);
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

  if (!m_active)
    return;

  Name objectName = interest->getName();

  if (Name("/hint").isPrefixOf(objectName))
  {
    RespondHint(interest);
  }
  else if (Name("/vicinity").isPrefixOf(objectName))
  {
    RespondVicinity(interest);
  }
  else
  {
    RespondInterest(interest);
  }

}
void
MobileUser::RespondHint(shared_ptr<const Interest> interest)
{
  Name objectName = interest->getName().getSubName(1, 2);
  uint32_t nodeId = interest->getNodeId();
  StrategySelectors incomingSelectors = interest->getStrategySelectors();

  /** CHECK HINTPROBING FLAG -> COMPARE THRESHOLDS -> SEND INTEREST
   * Test if hintProbing is active. Case it is not, test if Hint is being sent to the right user. 
   * During hintProbing, the Hint is flooded to the Vicinity, with no targeted user.
   * Only Consumers meeting the criteria can send back responses
   */

  if (m_hintProbing) {
    if (!incomingSelectors.empty()) {
      if (m_userInterested == incomingSelectors.getInterested() && m_userAvailability <= incomingSelectors.getAvailability()) {
        if (m_providedObjects.size() == m_cacheSize) {
          uint32_t pos = (uint32_t) m_rand->GetValue(0, m_cacheSize);
          Name removedObject = m_providedObjects[pos];
	  UnannounceContent(removedObject);
          FibHelper::RemoveRoute(GetNode(), removedObject, m_face);
          m_providedObjects.erase(m_providedObjects.begin() + pos);
        }

        NS_LOG_DEBUG("Node " << GetNode()->GetId() << " received hint and meets the criteria. Will now reques object " << objectName);

        AddInterestObject(objectName);
        m_providedObjects.push_back(objectName);
      }
      else {
        return;
      }
    }
  }
  
  else if (nodeId == this->GetNode()->GetId() ) {
    if (m_providedObjects.size() == m_cacheSize) {
      uint32_t pos = (uint32_t) m_rand->GetValue(0, m_cacheSize);
      Name removedObject = m_providedObjects[pos];
      UnannounceContent(removedObject);
      FibHelper::RemoveRoute(GetNode(), removedObject, m_face);
      m_providedObjects.erase(m_providedObjects.begin() + pos);
    }

    NS_LOG_DEBUG("Node " << GetNode()->GetId() << " accepted the hint to request object " << objectName);

    AddInterestObject(objectName);
    m_providedObjects.push_back(objectName);
  }
  
    
}

uint32_t
MobileUser::RespondVicinity(shared_ptr<const Interest> interest)
{
  Name objectName = interest->getName();

  // Create the vicinity packet
  shared_ptr<Data> vicinityData = make_shared<Data>();
  vicinityData->setName(objectName);
  vicinityData->setFreshnessPeriod(::ndn::time::milliseconds(0));

  // Create an empty strategy selector for the Vicinity Data packet 
  StrategySelectors responseSelectors = StrategySelectors();
  // Store strategy selectors from incoming Vicinity packet (Producer Thresholds)
  StrategySelectors incomingSelectors = interest->getStrategySelectors();
 
  NS_LOG_DEBUG("Node " << this->GetNode()->GetId() << " received vicinity probing with parameters " << incomingSelectors.getInterested() << " / " << incomingSelectors.getAvailability());
 
  /* Will call method to compare user parameters with incomingData parameters
    for now, the comparison is made directly in this method */

  /* First step, test incomingSelectors for default values.
       If the values are different, the user will send a response according
      to the thresholds set by the Producer. 
        If the Consumer is not Interested on providing that content, returns code 1 and does not send response.
        If the Consumer is Interested but does not meet the threshold, return 2 and does not send response.
        If Consumer is interested and meets the thresholds, return 0 and sends customized response.
       In case the selectors are default, the Producer doesnt need any information, return 0 and 
      sends defaults response. */

  if(!incomingSelectors.empty())
  {
    responseSelectors.setNodeId(this->GetNode()->GetId());
    
    // compare user interest with producer thresholds
    if(m_userInterested == incomingSelectors.getInterested())
    {
      responseSelectors.setInterested(m_userInterested);
    }
    else
    {
      return 1;
    }
    // compare user availability with producer threshold
    if(m_userAvailability <= incomingSelectors.getAvailability())
    {
      responseSelectors.setAvailability(m_userAvailability);
    }
    else
    {
      return 2;
    }
  }
  
  NS_LOG_DEBUG("Node " << this->GetNode()->GetId() << " responded vicinity probing with parameters " << responseSelectors.getInterested() << " / " << responseSelectors.getAvailability());
  
  vicinityData->setContent(responseSelectors.wireEncode());

  // Add signature
  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }

  signature.setInfo(signatureInfo);
  signature.setValue(::ndn::nonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));

  vicinityData->setSignature(signature);

  //NS_LOG_DEBUG("Node " << GetNode()->GetId() << " responded vicinity discovery for object " << vicinityData->getName());

  // Send the packet
  vicinityData->wireEncode();

  m_transmittedDatas(vicinityData, this, m_face);
  m_face->onReceiveData(*vicinityData);

  return 0;
}

void
MobileUser::RespondInterest(shared_ptr<const Interest> interest)
{
  Name dataName(interest->getName());

  // TO DO : CHECK HINTPROBING FLAG -> CHECK INCOMING INTEREST FOR SELECTORS -> PUSH CONSUMER INTO VICINITY
  /*if (m_hintProbing) {
    StrategySelectors incomingSelectors = interest->getStrategySelectors();
    
    if(!incomingSelectors.empty()) {
      UserInformation userInfo = UserInformation(incomingSelectors.getNodeId(), incomingSelectors.getAvailability(), incomingSelectors.getInterested());
      m_vicinity.push_back(userInfo);

      NS_LOG_DEBUG("Node " << GetNode()->GetId() << " vicinity is :");
      for(uint32_t i=0;i<m_vicinity.size();i++)
        NS_LOG_DEBUG("Node " << m_vicinity[i].GetNodeId() << " has availablity " << m_vicinity[i].GetAvailability());
     }
  } */
  
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

  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " responded interest with data for object " << data->getName());
  m_servedData(this, interest->getName());

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

  Name objectName = data->getName();

  if (Name("/vicinity").isPrefixOf(objectName))
  {
    RespondVicinityData(data);
  }
  else
  {
    RespondData(data);
  }
}

void
MobileUser::RespondVicinityData(shared_ptr<const Data> data)
{
  Block block = data->getContent();
  block.parse();
  StrategySelectors incomingSelectors = StrategySelectors(*block.elements_begin()); 
  
  UserInformation userInfo = UserInformation(incomingSelectors.getNodeId(), incomingSelectors.getAvailability(), incomingSelectors.getInterested());
 
  /* Pushes Consumer directly into the Vicinity. If the probingModel is set to ranked, sorting
    is performed later during Device Selection. */
  NS_LOG_DEBUG("Node " << incomingSelectors.getNodeId() << " was added to node " << GetNode()->GetId() << " vicinity");
  m_vicinity.push_back(userInfo);
 
  /*
    ALTERNATIVE : this strategy consider an ordered insertion 
     Depending on the probing model the producer will deal with the
      Vicinity Response accordingly.
  
  if(!m_probingModel.compare("ranked"))
  { 
    NS_LOG_DEBUG("Node " << incomingSelectors.getNodeId() << " was added to node " << GetNode()->GetId() << " vicinity");
    
    // Will call method for ranked insertion on the vicinity (binary insertion)
    m_vicinity.push_back(incomingSelectors.getNodeId());
  }
  
  if(!m_probingModel.compare("random"))
  {
    NS_LOG_DEBUG("Node " << incomingSelectors.getNodeId() << " was added to node " << GetNode()->GetId() << " vicinity");
    m_vicinity.push_back(incomingSelectors.getNodeId());
  } */
}

void
MobileUser::RespondData(shared_ptr<const Data> data)
{
  Name objectName = data->getName();
 
  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " got data packet for object " << objectName);
 
  // Update the pending interest requests
  m_pendingObjects.remove(objectName);
  m_pendingInterests--;

  Name objectPrefix = objectName.getSubName(0, 2);
    
  //m_windowSize = m_windowSize + 0.25;

  // Check if it is the last chunk
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
  {
    ConcludeObjectDownload(objectPrefix);
  }

  // Calculate the hop count
  int hopCount = 0;
  auto ns3PacketTag = data->getTag<Ns3PacketTag>();
  if (ns3PacketTag != nullptr) { 
    FwHopCountTag hopCountTag;
    if (ns3PacketTag->getPacket()->PeekPacketTag(hopCountTag)) {
      hopCount = hopCountTag.Get();
      //NS_LOG_DEBUG("Hop count: " << hopCount);
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
  ScheduleNextInterestPacket(false);
}

/**
 * Announcement packet handler.
 * The application should not receive it.
 * Do nothing.
 */ 
//void
//MobileUser::OnAnnouncement(shared_ptr<const Announcement> announcement)
//{
//  App::OnAnnouncement(announcement);
//}

/**
 * Interest packet timeout handler.
 * Update retransmission and rtt statistics.
 * Update the pending interest data structures.
 * Schedule to send a new Interest.  
 */
void
MobileUser::OnTimeout(Name objectName)
{
  //NS_LOG_FUNCTION(objectName);
  //std::cout << Simulator::Now () << ", TO: " << objectName << ", current RTO: " <<
  //             m_rtt->RetransmitTimeout ().ToDouble (Time::S) << "s\n";


  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " timed out an interest for object " << objectName << " with a period of " << m_rtt->RetransmitTimeout().ToDouble(Time::S) << "s");

//  m_rtt->IncreaseMultiplier();
  m_rtt->SentSeq(m_chunkOrder[objectName], 1);
  m_retxNames.push_back(objectName);
  
  m_pendingObjects.remove(objectName);
  m_pendingInterests--;

  //Name objectPrefix = objectName.getSubName(0, 2);
  //m_windowSize = (m_windowSize*7.0) / 8.0;
  //if (m_windowSize < 1)
  //{
  //  m_windowSize = 1;
  //}

//  ScheduleNextInterestPacket(true);
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
MobileUser::SendInterestPacket(bool timeout)
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

  if(m_hintProbing) {
    interest->setAvailability(m_userAvailability);
    interest->setInterested(m_userAvailability);
  }

  // Send packet
  WillSendOutInterest(object);

  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);

  // Pending objects control
  m_pendingObjects.push_back(interest->getName());
  m_pendingInterests++;

  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " sent an INTEREST for object " << object);

  ScheduleNextInterestPacket(timeout);
}

/**
 * Update the control data structures of the interest packet. 
 */
void
MobileUser::WillSendOutInterest(Name objectName)
{
  //NS_LOG_DEBUG("Trying to add " << objectName << " with " << Simulator::Now().ToDouble(Time::S) << " seconds. Already "
  //                              << m_nameTimeouts.size() << " items");

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
MobileUser::ScheduleNextInterestPacket(bool timeout)
{
  if (m_moving)
  {
    m_movingInterest = true;
    return;
  }

  if (m_interestQueue.size() > 0 || m_retxNames.size() > 0) {
    if (m_pendingInterests < m_windowSize) {
      if (m_sendEvent.IsRunning()) {
        Simulator::Remove(m_sendEvent);
      }
      if (timeout)
      {
        Time delay = m_rtt->RetransmitTimeout();
        m_sendEvent = Simulator::Schedule(delay, &MobileUser::SendInterestPacket, this, timeout);
      }
      else
      {
        m_sendEvent = Simulator::ScheduleNow(&MobileUser::SendInterestPacket, this, timeout);
      }
    }
  }
}

/**
 * Adds a new object to the interest queue.
 * Creates the name of each chunk to be requested
 * and puts them into the interest queue.
 */
void
MobileUser::AddInterestObject(Name objectName)
{
  for (uint32_t i = 0; i < m_downloadedObjects.size(); i++) {
    if (objectName == m_downloadedObjects[i]) {
      return;
    }
  }

  objectProperties properties = m_catalog->getObjectProperties(objectName);
  shared_ptr<Name> interestName;

  // For each chunk, append the sequence number and add it to the interest queue
  for (uint32_t i = 0; i < properties.size; i++) {
    interestName = make_shared<Name>(objectName);
    interestName->appendSequenceNumber(i);
    m_interestQueue.push_back(*interestName);
  }
  
  m_downloadedObjects.push_back(objectName);

  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " starting download of object " << objectName);

  Simulator::ScheduleNow(&MobileUser::ScheduleNextInterestPacket, this, false);
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
  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " concluded download of object " << objectName);

  for (uint32_t i = 0; i < m_providedObjects.size(); i++) {
    if (objectName == m_providedObjects[i]) {
      AnnounceContent(objectName, true);
      FibHelper::AddRoute(GetNode(), objectName, m_face, 0);
      break;
    }
  }

  //m_windowSize = m_initialWindowSize;

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
//  for (uint32_t i = 0; i < m_generatedContent.size(); i++) {
//    AnnounceContent(m_generatedContent[i], false);
//  }
  for (uint32_t i = 0; i < m_providedObjects.size(); i++) {
    AnnounceContent(m_providedObjects[i], false);
  }
//  if (m_providedObjects.size() > 0) {
//    ndn::GlobalRoutingHelper::CalculateRoutes();
//    ndn::GlobalRoutingHelper::PrintFIBs();
//  }
}

/**

 * Announce a specific content object.
 * Creates an announcement packet and sends it.
 */
void
MobileUser::AnnounceContent(Name object, bool update)
{
  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " announced object " << object << " at " << m_position.x);
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
//  ndnGlobalRoutingHelper.InstallAll();  
  ndnGlobalRoutingHelper.AddOrigin(object.toUri(), this->GetNode());
  
  if (update) {
    ndn::GlobalRoutingHelper::CalculateRoutes();
    ndn::GlobalRoutingHelper::PrintFIBs();
  }
}

void
MobileUser::UnannounceContent(Name object)
{
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
//  ndnGlobalRoutingHelper.InstallAll();  
  ndnGlobalRoutingHelper.RemoveOrigin(object.toUri(), GetNode());

  ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::PrintFIBs();
}

// PRODUCER ACTIONS

//void
//MobileUser::ExpireContent(Name expiredObject)
//{
//  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " expired object " << expiredObject);
//
//  for (uint32_t i = 0; i < m_generatedContent.size(); i++) {
//    if (expiredObject == m_generatedContent[i]) {
//      m_generatedContent.erase(m_generatedContent.begin()+i);
//      break;
//    }
//  }
//  m_catalog->removeObject(expiredObject);
//
//  Simulator::Schedule(m_publishTime, &MobileUser::PublishContent, this);
//}

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
  if (m_moving)
  {
    m_movingPublish = true;
    return;
  }

  Name newObject = CreateContentName();
  m_lastObject = newObject;

  GenerateContent(newObject);

  AdvertiseContent(newObject);

  //AnnounceContent(newObject, false);

  if (m_vicinitySize > 0 && !m_hintProbing)
  {
    ProbeVicinity(newObject);
  }

  if (m_replicationDegree > 0)
  {
    Simulator::Schedule(m_vicinityTimer, &MobileUser::PushContent, this, newObject);
  }

  Simulator::Schedule(m_publishTime, &MobileUser::PublishContent, this);

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

void
MobileUser::GenerateContent(Name object)
{
  //NS_LOG_DEBUG("Node " << GetNode()->GetId() << " published object " << object << " " << m_popularity);
  m_catalog->addObject(object, m_popularity);

  objectProperties p = m_catalog->getObjectProperties(object);
 
  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " published object " << object << " " << p.popularity << " / " << p.size);
}

/**
 * Advertise a new content to other consumers.
 * Based on the popularity of the content, schedule consumers to request it.
 */
void
MobileUser::AdvertiseContent(Name newObject)
{
  vector<Ptr<Node>> users = m_catalog->getUsers();
  objectProperties properties = m_catalog->getObjectProperties(newObject);
  Time maxSimulationTime = m_catalog->getMaxSimulationTime();
  Ptr<Node> currentUser;
  Ptr<MobileUser> mobileUser;

  for (uint32_t i = 0; i < users.size(); i++) {
    Ptr<Node> currentUser = users[i];

    // If not the publisher
    if (currentUser->GetId() != this->GetNode()->GetId() && m_rand->GetValue(0, 1) < properties.popularity) {
      Ptr<MobileUser> mobileUser = DynamicCast<MobileUser> (currentUser->GetApplication(0));
      double requestTime = m_rand->GetValue(0, (maxSimulationTime - Simulator::Now()).ToDouble(Time::S));

      Simulator::ScheduleWithContext(currentUser->GetId(), Time(to_string(requestTime) + "s"), &MobileUser::AddInterestObject, mobileUser, newObject);
    }
  }

//  Simulator::Schedule(Seconds(properties.lifetime), &MobileUser::ExpireContent, this, newObject);
}

// STRATEGY ACTIONS

/**
 * Triggers the vicinity discovery process
 * Sends a vicinity packet with a given scope.
 */
void
MobileUser::ProbeVicinity(Name object)
{
  m_vicinity.clear();

  // Create the vicinity packet
  Name vicinityName = Name("/vicinity");
  vicinityName.append(object);

  shared_ptr<Interest> vicinity = make_shared<Interest>();
  vicinity->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  vicinity->setName(vicinityName);
  time::milliseconds vicinityLifeTime(m_vicinityTimer.GetMilliSeconds());
  vicinity->setInterestLifetime(vicinityLifeTime);

  vicinity->setScope(m_vicinitySize);

  // In this step the vicinity packet is set with the producer threshold
  // Threshold values will determine the type of probing performed (ranked, limited ranked, random)
  if(!m_probingModel.compare("ranked"))
  {
    vicinity->setInterested(m_interestedThreshold);
    vicinity->setAvailability(m_availabilityThreshold);
  }
  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " sent vicinity probing packet " << vicinity->getName());
  
  // Send the packet
  vicinity->wireEncode();

  m_transmittedInterests(vicinity, this, m_face);
  m_face->onReceiveInterest(*vicinity);
}

/**
 * Push the content to other devices.
 * Select the devices on the learnt vicinity
 * and send them the content.
 */
void
MobileUser::PushContent(Name objectName)
{
  if (m_moving)
  {
    m_movingPush = true;
    return;
  }

  if (!m_hintProbing) {
    if (m_vicinity.size() > 0) {
      int deviceId;

      if (m_probingModel == "ranked")
      {  
        SortVicinity();  
        for (uint32_t i = 0; i < m_replicationDegree; i++) {
          deviceId = m_vicinity[i].GetNodeId();
          HintContent(deviceId, objectName);
        }
      }

      else if(m_probingModel == "random")
      {
        for (uint32_t i = 0; i < m_replicationDegree; i++) {
          deviceId = SelectRandomDevice(); 
          HintContent(deviceId, objectName);
        }
      }
    }
  }
  else {
    /**
     * If hintProbing is active, Producer will flood its scope with hints containing parameters thresholds
     * and only Consumers that meet the criteria will send responses.
    */
    HintContent(-1, objectName);
  }
}

/**
 * @brief Basic compare function to compare two elements during qsort algorithm
 */

static int compare (const void* p1, const void* p2)
{
  return ( (*(UserInformation*)p1).GetAvailability() - (*(UserInformation*)p2).GetAvailability() ); 
}

/**
 * Sort the vicinity based on device availability. 
 */
void
MobileUser::SortVicinity()
{
  qsort(m_vicinity.data(), m_vicinity.size(), sizeof(UserInformation), compare);
  NS_LOG_DEBUG("Vicinity from node " << this->GetNode()->GetId());
  for(uint32_t k=0;k<m_vicinity.size();k++)
    NS_LOG_DEBUG("Node " << m_vicinity[k].GetNodeId() << " has availability " << m_vicinity[k].GetAvailability());
}

/**
 * Select a random device from the vicinity.
 */
int
MobileUser::SelectRandomDevice()
{
  int position = m_rand->GetValue(0, m_vicinity.size());
  return m_vicinity[position].GetNodeId();
}

/**
 * Send (push) the content to others.
 * We implement it as a hint instead of unsolicited data.
 */
void
MobileUser::HintContent(int deviceId, Name objectName)
{
  m_vicinity.clear();

  // Create the vicinity packet
  Name hintName = Name("/hint");
  hintName.append(objectName);

  shared_ptr<Interest> hint = make_shared<Interest>();
  hint->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  hint->setName(hintName);
  time::milliseconds hintLifeTime(m_hintTimer.GetMilliSeconds());
  hint->setInterestLifetime(hintLifeTime);
  hint->setScope(m_vicinitySize);
  
  if(m_hintProbing)
  {
    hint->setAvailability(m_availabilityThreshold);
    hint->setInterested(m_interestedThreshold);
    NS_LOG_DEBUG("Node " << GetNode()->GetId() << " sent Hint probe " << hint->getName() << " for  all devices in " << hint->getScope() << " hops");
  }
  else {
    hint->setNodeId(deviceId);
    NS_LOG_DEBUG("Node " << GetNode()->GetId() << " hinted content " << hint->getName() << " for device " << hint->getNodeId() << " in " << hint->getScope() << " hops");
  }

  // Send the packet
  hint->wireEncode();

  m_transmittedInterests(hint, this, m_face);
  m_face->onReceiveInterest(*hint);

}

// MOBILITY ACTIONS

/**
 *
 */
void
MobileUser::Move(Ptr<const MobilityModel> model)
{
  m_moving = true;

  vector<Ptr<Node>> routers = m_catalog->getRouters();
  Ptr<Node> currentRouter;
  Ptr<MobilityModel> mob;

//  for (uint32_t i = 0; i < routers.size(); i++) {
//    Ptr<Node> currentRouter = routers[i];
  ndn::LinkControlHelper::FailLink(this->GetNode(), routers[m_position.x]);
//  }

  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " started movement");
}

/**
 *
 */
void
MobileUser::Session(Ptr<const MobilityModel> model)
{
  m_moving = false;
 
  m_position = model->GetPosition();

  vector<Ptr<Node>> routers = m_catalog->getRouters();
  Ptr<Node> currentRouter;
  Ptr<MobilityModel> mob;
      
  ndn::LinkControlHelper::UpLink(this->GetNode(), routers[m_position.x]);
/*
  for (uint32_t i = 0; i < routers.size(); i++) {
    Ptr<Node> currentRouter = routers[i];

    if (currentRouter->GetId() == pos.x) {
      ndn::LinkControlHelper::UpLink(this->GetNode(), currentRouter);
    }
  }
*/
  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " started session at router " << m_position.x);

  AnnounceContent(); 
  
  if (m_movingInterest)
  {
    Simulator::ScheduleNow(&MobileUser::ScheduleNextInterestPacket, this, false);
  }

  if (m_interestQueue.size() > 0 || m_retxNames.size() > 0)
  {
    Simulator::ScheduleNow(&MobileUser::ScheduleNextInterestPacket, this, false);
  }

  if (m_movingPublish)
  {
    Simulator::ScheduleNow(&MobileUser::PublishContent, this);
  }

  if (m_movingPush)
  {
    Simulator::ScheduleNow(&MobileUser::PushContent, this, m_lastObject);
  }

}

void
MobileUser::CourseChange(Ptr<const MobilityModel> model)
{
  if (m_moving)
  {
    Session(model);
  }
  else
  {
    Move(model);
    m_movingInterest = false;
    m_movingPublish = false;
    m_movingPush = false;
  }

  ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::PrintFIBs();
}


/**
 * @brief Constructor for UserInformation
 */

UserInformation::UserInformation(uint32_t nodeId, uint32_t availability, bool interested)
{
  m_nodeId = nodeId;
  m_availability = availability;
  m_interested = interested;
}

} // namespace ndn
} // namespace ns3


