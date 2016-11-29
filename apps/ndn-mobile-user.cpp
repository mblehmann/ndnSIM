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
 * @author Lucas Leal <lsleal@inf.ufrgs.br>
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

#include "helper/ndn-global-routing-helper.hpp"
#include "helper/ndn-link-control-helper.hpp"

#include "ndn-cxx/pdrm-strategy-selectors.hpp"

#include "stdlib.h"
#include "time.h"

#include <boost/lexical_cast.hpp>
#include <boost/ref.hpp>

#include <list>

using namespace std;

NS_LOG_COMPONENT_DEFINE("ndn.MobileUser");

namespace ns3 {
namespace ndn {

const Time DEFAULT_VICINITY_TIMER = Time("5s"); // 5 seconds
const Time DEFAULT_HINT_TIMER = Time("5s"); // 5 seconds
const uint32_t DEFAULT_PLACEMENT_POLICY = 0; // 0 = smart; 1 = random
const uint32_t DEFAULT_VICINITY_SIZE = 0; // 0 hops of distance (does not execute strategy by default)
const uint32_t DEFAULT_CACHE_SIZE = 0; // 0 chunks (does not execute strategy by default)

const uint32_t DEFAULT_WINDOW_SIZE = 1; // 1 request at a time
const Time DEFAULT_INTEREST_LIFETIME = Time("2s"); // 2 seconds
const Time DEFAULT_RETRANSMISSION_TIMER = Time("50ms"); // 50 milliseconds

const string DEFAULT_PREFIX = "/producer"; // adds the node ID
const string DEFAULT_OBJECT_PREFIX = "/object"; // adds the object ID
const uint32_t DEFAULT_PAYLOAD_SIZE = 1024; // 1024 bytes (1kb) per chunk
const Time DEFAULT_FRESHNESS = Time("0s"); // 0 seconds => unlimited freshness
const Time DEFAULT_PRODUCE_TIME = Time("0s"); // 0 seconds => do not produce content

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
      .AddAttribute("WindowSize", "Window size",
                    UintegerValue(DEFAULT_WINDOW_SIZE),
                    MakeUintegerAccessor(&MobileUser::m_windowSize),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("RetxTimer",
                    "Timeout defining how frequent retransmission timeouts should be checked",
                    TimeValue(DEFAULT_RETRANSMISSION_TIMER),
                    MakeTimeAccessor(&MobileUser::GetRetxTimer, &MobileUser::SetRetxTimer),
                    MakeTimeChecker())

      // Strategy
      .AddAttribute("PlacementPolicy", "Placement Policy",
                    UintegerValue(DEFAULT_PLACEMENT_POLICY),
                    MakeUintegerAccessor(&MobileUser::m_placementPolicy),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("VicinitySize", "Vicinity size",
                    UintegerValue(DEFAULT_VICINITY_SIZE),
                    MakeUintegerAccessor(&MobileUser::m_vicinitySize),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("VicinityTimer", "Period to discover vicinity",
                    TimeValue(DEFAULT_VICINITY_TIMER),
                    MakeTimeAccessor(&MobileUser::m_vicinityTimer),
                    MakeTimeChecker())

      .AddAttribute("HintTimer", "Lifetime of the hint",
                    TimeValue(DEFAULT_HINT_TIMER),
                    MakeTimeAccessor(&MobileUser::m_hintTimer),
                    MakeTimeChecker())

      .AddAttribute("CacheSize", "Number of content provided on behalf of others",
                    UintegerValue(DEFAULT_CACHE_SIZE),
                    MakeUintegerAccessor(&MobileUser::m_cacheSize),
                    MakeUintegerChecker<uint32_t>())

      // Global
      .AddAttribute("Catalog", "Content catalog",
                    PointerValue(NULL),
                    MakePointerAccessor(&MobileUser::m_catalog),
                    MakePointerChecker<Catalog>())

      // Producer
      .AddAttribute("ProduceTime", "Period between publication of new objects",
                    TimeValue(DEFAULT_PRODUCE_TIME),
                    MakeTimeAccessor(&MobileUser::m_produceTime),
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
  , m_rtt(CreateObject<RttMeanDeviation>())
  , m_windowSize(DEFAULT_WINDOW_SIZE)
  , m_interestLifeTime(DEFAULT_INTEREST_LIFETIME)
  , m_prefix(DEFAULT_PREFIX)
  , m_postfix(DEFAULT_OBJECT_PREFIX)
  , m_virtualPayloadSize(DEFAULT_PAYLOAD_SIZE)
  , m_freshness(DEFAULT_FRESHNESS)
  , m_moving(false)
  , m_movingRequesting(false)
  , m_movingProducing(false)
  , m_movingReplicating(false)
  , m_usedCache(0)
{
}

/**
 * Operations executed at the start of the application.
 * The mobile user will register its prefix in the interface.
 */
void
MobileUser::StartApplication() 
{
  App::StartApplication();
  // add FIB entriy to the application face (so that the application can receive these messages)
  FibHelper::AddRoute(GetNode(), "/hint", m_face, 0);
  FibHelper::AddRoute(GetNode(), "/vicinity", m_face, 0);

  // initialize mobility and the signals for events  
  Ptr<MobilityModel> mob = this->GetNode()->GetObject<MobilityModel>();
  mob->TraceConnectWithoutContext("CourseChange", MakeCallback(&MobileUser::CourseChange, this));

  // initialize the position
  m_position = mob->GetPosition();
  vector<Ptr<Node>> routers = m_catalog->getRouters();
  ndn::LinkControlHelper::UpLink(this->GetNode(), routers[m_position.x]);

  // schedule a global route calculation
  //ndn::GlobalRoutingHelper::CalculateRoutes();

  // initialize the user popularity
  m_producerPopularity = m_catalog->getUserPopularity();

  // Set Availability and Interested attributes for each user
  //m_userAvailability = 1;
  m_homeNetwork = m_position.x;
  m_movementPeriod = Time("0s");
  m_sessionPeriod = Time("0s");
  m_userAvailability = 0;
  m_lastMovementEvent = Simulator::Now();

  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " initial position at router " << m_position.x << " with popularity " << m_producerPopularity << " and availability " << m_userAvailability );

  // if it is a publisher, publish something!
  if (!m_produceTime.IsZero())
  {
    // update the producer prefix with the node ID
    m_prefix = m_prefix.toUri() + to_string(GetNode()->GetId());
    FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
  
    ndn::GlobalRoutingHelper::CalculateRoutes();

    Simulator::ScheduleNow(&MobileUser::ProduceContent, this);
  }

  // schedule the endgame
  Simulator::Schedule(m_catalog->getMaxSimulationTime() - Time("1ms"), &MobileUser::EndGame, this);
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
  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " concluded end game with " << (m_chunkRequestQueue.size() + m_pendingChunks.size() + m_retxChunksQueue.size()) << " unretrieved requests");
  // ML TODO: register donwload time per object and other interesting statistics. Build a tracer that records it.
}

// RETRANSMISSION CONTROL

/**
 * Set the retransmission check timer.
 * It is executed once in the beginning of the simulation and checks periodically afterwards.
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

  pair<Name, Time> entry;

  while (!m_chunkTimeouts.empty()) {

    // looks for the closest expiring interest entry
    entry = make_pair(m_chunkTimeouts.begin()->first, m_chunkTimeouts.begin()->second);
    
    for (map<Name, Time>::iterator it = m_chunkTimeouts.begin(); it != m_chunkTimeouts.end(); ++it)
    {
      if (it->second < entry.second)
      {
        entry = make_pair(it->first, it->second);
      }   
    }

    // if it is timed out, process, and repeat the loop to find if the next one also expired
    if (entry.second + rto <= now)
    {
      Name expiredInterest = entry.first;
      m_chunkTimeouts.erase(entry.first);
      OnTimeout(expiredInterest);
    }
    // otherwise, the others will not be expired
    else
      break;
  }

  m_retxEvent = Simulator::Schedule(m_retxTimer, &MobileUser::CheckRetxTimeout, this);
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
  //std::cout << Simulator::Now () << ", TO: " << objectName << ", current RTO: " <<
  //             m_rtt->RetransmitTimeout ().ToDouble (Time::S) << "s\n";

  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " timed out an interest for object " << objectName << " with a period of " << m_rtt->RetransmitTimeout().ToDouble(Time::S) << "s");

  m_rtt->IncreaseMultiplier();
  m_rtt->SentSeq(m_chunkOrder[objectName], 1);
  m_retxChunksQueue.push(objectName);
 
  if (m_pendingChunks.count(objectName) > 0)
    m_pendingChunks.erase(objectName);
 
  ScheduleNextInterestPacket();
}

// PACKET/EVENT HANDLERS

/**
 * Interest packet handler.
 * Receive the interest packet and checks which type is: hint, vicinity, or regular
 * Process accordingly
 */
void
MobileUser::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest);
  if (!m_active)
    return;

  NS_LOG_DEBUG("NODE " << GetNode()->GetId() << " RECEIVED INTEREST " << interest->getName() << " SCOPE: " << interest->getScope());

  Name objectName = interest->getName();

  if (Name("/hint").isPrefixOf(objectName)) RespondHint(interest);
  else if (Name("/vicinity").isPrefixOf(objectName)) RespondVicinity(interest);
  else RespondInterest(interest);
}

/**
 * Respond to Interest packets
 * The producer creates a data packet and sends to the consumer
 */
void
MobileUser::RespondInterest(shared_ptr<const Interest> interest)
{ 
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
  
  // logging and stats
  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " responded interest with data for object " << data->getName());
  m_servedData(this, interest->getName());
  
  // Send it
  data->wireEncode();
  
  m_transmittedDatas(data, this, m_face);
  m_face->onReceiveData(*data);
}

/*
 * Respond to a hint packet
 * A hint aims for a specific user ID to request the content
 */
void
MobileUser::RespondHint(shared_ptr<const Interest> interest)
{
  PDRMStrategySelectors incomingSelectors = interest->getPDRMStrategySelectors();

  // check if the node is the target
  if (incomingSelectors.getNodeId() == GetNode()->GetId())
  {
    Name objectName = interest->getName().getSubName(1, 2);
    uint32_t objectSize = (m_catalog->getObjectProperties(objectName)).size;

    NS_LOG_DEBUG("Node " << GetNode()->GetId() << " accepted the hint to request object " << objectName);

    StartObjectDownload(objectName);
    StoreObject(objectName, objectSize);
  }

}

/*
 * Respond to a vicinity probing
 * Checks if the node meets the requirement and send a vicinity data packet as response
 */
void
MobileUser::RespondVicinity(shared_ptr<const Interest> interest)
{
  Name objectName = interest->getName().getSubName(1, 2);
  PDRMStrategySelectors incomingSelectors = interest->getPDRMStrategySelectors();
  
  bool interested = m_interestingObjects.count(objectName);
  if (m_sessionPeriod + m_movementPeriod == Time("0s"))
    m_userAvailability = 0;
  else
    m_userAvailability = m_sessionPeriod / (m_sessionPeriod + m_movementPeriod);

  Name dataName = interest->getName();

  // Create the vicinity packet
  shared_ptr<Data> vicinityData = make_shared<Data>();
  vicinityData->setName(dataName);
  vicinityData->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

  // Create an empty strategy selector for the Vicinity Data packet 
  PDRMStrategySelectors responseSelectors = PDRMStrategySelectors();

  responseSelectors.setNodeId(GetNode()->GetId());
  responseSelectors.setInterest(interested);
  responseSelectors.setAvailability(m_userAvailability);
  responseSelectors.setHomeNetwork(m_homeNetwork);

  NS_LOG_DEBUG("Node " << this->GetNode()->GetId() << " responded vicinity probing with parameters " << responseSelectors.getInterest() << " / " << responseSelectors.getAvailability() << " / " << responseSelectors.getHomeNetwork());
  
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

  // Send the packet
  vicinityData->wireEncode();

  m_transmittedDatas(vicinityData, this, m_face);
  m_face->onReceiveData(*vicinityData);
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

  if (Name("/vicinity").isPrefixOf(objectName)) RespondVicinityData(data);
  else RespondData(data);
}

void
MobileUser::RespondData(shared_ptr<const Data> data)
{
  Name objectName = data->getName();
 
  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " got data packet for object " << objectName);
 
  // Update the pending interest requests

  if (m_pendingChunks.count(objectName) > 0)
    m_pendingChunks.erase(objectName);

  Name objectPrefix = objectName.getSubName(0, 2);
  m_downloadedChunks[objectPrefix]++;

  if (m_downloadedChunks[objectPrefix] == m_objectSizes[objectPrefix])
    ConcludeObjectDownload(objectPrefix);

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
  Time lastDelay = m_chunkLastDelay[objectName];
  m_lastRetransmittedInterestDataDelay(this, objectName, Simulator::Now() - lastDelay, hopCount);

  Time fullDelay = m_chunkFullDelay[objectName];
  m_firstInterestDataDelay(this, objectName, Simulator::Now() - fullDelay, m_chunkRetxCounts[objectName], hopCount);

  m_chunkRetxCounts.erase(objectName);
  m_chunkFullDelay.erase(objectName);
  m_chunkLastDelay.erase(objectName);

  m_chunkTimeouts.erase(objectName);

  m_rtt->AckSeq(m_chunkOrder[objectName]);

  // Schedule next packets
  ScheduleNextInterestPacket();
}

void
MobileUser::RespondVicinityData(shared_ptr<const Data> data)
{
  Block block = data->getContent();
  block.parse();
  PDRMStrategySelectors incomingSelectors = PDRMStrategySelectors(*block.elements_begin());

  UserInformation userInfo = UserInformation(incomingSelectors.getNodeId(), incomingSelectors.getHomeNetwork(), incomingSelectors.getAvailability(), incomingSelectors.getInterest());

  /* Pushes Consumer directly into the Vicinity. If the probingModel is set to ranked, sorting
    is performed later during Device Selection. */
  NS_LOG_DEBUG("Node " << incomingSelectors.getNodeId() << " was added to node " << GetNode()->GetId() << " vicinity");
  m_vicinity.push_back(userInfo);

}

// CONSUMER ACTIONS

/**
 * Adds a new object to the interest queue.
 * Creates the name of each chunk to be requested
 * and puts them into the interest queue.
 */
void
MobileUser::FoundObject(Name objectName)
{
  // already downloading the object
  if (m_interestingObjects.count(objectName) > 0)
    return;

  m_interestingObjects.insert(objectName);
  Time maxSimulationTime = m_catalog->getMaxSimulationTime();

  double requestTime = m_rand->GetValue(0, (maxSimulationTime - Simulator::Now()).ToDouble(Time::S));

  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " found object " << objectName << " and will start downloading at " << (Simulator::Now() + Time(to_string(requestTime) + "s")));
  Simulator::Schedule(Time(to_string(requestTime) + "s"), &MobileUser::StartObjectDownload, this, objectName);
}

/**
 * Adds a new object to the interest queue.
 * Creates the name of each chunk to be requested
 * and puts them into the interest queue.
 */
void
MobileUser::StartObjectDownload(Name objectName)
{
  if (m_requestedObjects.count(objectName) > 0)
    return;

  m_requestedObjects.insert(objectName);

  objectProperties properties = m_catalog->getObjectProperties(objectName);
  shared_ptr<Name> interestName;

  // For each chunk, append the sequence number and add it to the interest queue
  for (uint32_t i = 0; i < properties.size; i++) {
    interestName = make_shared<Name>(objectName);
    interestName->appendSequenceNumber(i);
    m_chunkRequestQueue.push(*interestName);
  }

  m_objectSizes[objectName] = properties.size;
  m_downloadedChunks[objectName] = 0;
  
  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " starting download of object " << objectName);

  Simulator::ScheduleNow(&MobileUser::ScheduleNextInterestPacket, this);
}

/**
 * Schedule the next Interest packet.
 * Checks if there are pending requests
 * and if it can send them (request window).
 */
void
MobileUser::ScheduleNextInterestPacket()
{
  if (m_moving)
  {
    m_movingRequesting = true;
    return;
  }

  if (m_chunkRequestQueue.size() + m_retxChunksQueue.size() > 0) {
    if (m_pendingChunks.size() < m_windowSize) {
      if (m_sendEvent.IsRunning()) {
        Simulator::Remove(m_sendEvent);
      }
      m_sendEvent = Simulator::ScheduleNow(&MobileUser::SendInterestPacket, this);
    }
  }
}

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
  if (m_retxChunksQueue.size() > 0) {
    object = m_retxChunksQueue.front();
    m_retxChunksQueue.pop();
  }
  else if (m_chunkRequestQueue.size() > 0) {
    object = m_chunkRequestQueue.front();
    m_chunkRequestQueue.pop();
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
  m_pendingChunks.insert(interest->getName());

  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " sent an INTEREST for object " << object);

  ScheduleNextInterestPacket();
}

/**
 * Update the control data structures of the interest packet. 
 */
void
MobileUser::WillSendOutInterest(Name objectName)
{
  //NS_LOG_DEBUG("Trying to add " << objectName << " with " << Simulator::Now().ToDouble(Time::S) << " seconds. Already "
  //                              << m_nameTimeouts.size() << " items");

  m_chunkTimeouts[objectName] = Simulator::Now();
  m_chunkFullDelay[objectName] = Simulator::Now();

  m_chunkLastDelay.erase(objectName);
  m_chunkLastDelay[objectName] = Simulator::Now();

  m_chunkRetxCounts[objectName]++;

  m_rtt->SentSeq(m_chunkOrder[objectName], 1);
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

  m_downloadedObjects.insert(objectName);
  m_objectSizes.erase(objectName);
  m_downloadedChunks.erase(objectName);

  if (m_providedObjects.count(objectName)) {
    AnnounceContent(objectName);
  }

}

// PROVIDER ACTIONS

/**
 * Announce a specific content object.
 * Creates an announcement packet and sends it.
 */
void
MobileUser::AnnounceContent(Name object)
{
  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " announced object " << object << " at " << m_position.x);
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.AddOrigin(object.toUri(), this->GetNode());

  FibHelper::AddRoute(GetNode(), object, m_face, 0);
  
  ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::PrintFIBs();
}

void
MobileUser::UnannounceContent(Name object)
{
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.RemoveOrigin(object.toUri(), GetNode());

  FibHelper::RemoveRoute(GetNode(), object, m_face);
  m_providedObjects.erase(object);

  ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::PrintFIBs();
}

void
MobileUser::StoreObject(Name object, uint32_t size)
{
  if (m_usedCache + size > m_cacheSize)
    DeleteObject(m_usedCache + size - m_cacheSize); // remove to make room for new object

  m_providedObjects[object] = size;
  m_usedCache += size;
}

void
MobileUser::DeleteObject(int32_t amount)
{
  pair<Name, uint32_t> entry;

  while (amount > 0)
  {
    // looks for the closest expiring interest entry
    entry = make_pair(m_providedObjects.begin()->first, m_providedObjects.begin()->second);

    for (map<Name, uint32_t>::iterator it = m_providedObjects.begin(); it != m_providedObjects.end(); ++it)
    {
      if (it->second > entry.second)
      {
        entry = make_pair(it->first, it->second);
      }
    }

    UnannounceContent(entry.first);
    amount -= entry.second;
    m_usedCache -= entry.second;
  }

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
//  Simulator::Schedule(m_produceTime, &MobileUser::ProduceContent, this);
//}

/**
 * Produces a new content object in the network.
 * Defines the name as /producer_prefix/object<index>/
 * Advertise the content to other consumers
 * and schedule their requests (based on popularity).
 * Announce the content to the routers.
 * Triggers the pushing strategy by discovering the vicinity
 * and schedules an event to push the object.
 */
void
MobileUser::ProduceContent()
{
  if (m_moving)
  {
    m_movingProducing = true;
    return;
  }

  m_lastProducedObject = PublishContent();

  if (m_vicinitySize > 0)
  {
    ReplicateContent(m_lastProducedObject);    
  }

  Simulator::Schedule(m_produceTime, &MobileUser::ProduceContent, this);

}

Name
MobileUser::PublishContent()
{
  //get content name
  int objectIndex = m_producedObjects.size();

  if (objectIndex == 0)
  {
    ndn::GlobalRoutingHelper::CalculateRoutes();
    ndn::GlobalRoutingHelper::PrintFIBs();
  }

  shared_ptr<Name> newObject = make_shared<Name>(m_prefix);
  newObject->append(m_postfix.toUri() + to_string(objectIndex));

  m_producedObjects.insert(*newObject);

  //generate content
  m_catalog->addObject(*newObject, m_producerPopularity);
  objectProperties p = m_catalog->getObjectProperties(*newObject);

  //advertise it
  vector<Ptr<Node>> users = m_catalog->getUsers();
  uint32_t consumers = 0;

  for (uint32_t i = 0; i < users.size(); i++) {
    Ptr<Node> currentUser = users[i];

    // If not the publisher
    if (currentUser->GetId() != this->GetNode()->GetId() && m_rand->GetValue(0, 1) < p.popularity) {
      Ptr<MobileUser> mobileUser = DynamicCast<MobileUser> (currentUser->GetApplication(0));
      consumers++;
      Simulator::ScheduleWithContext(currentUser->GetId(), Time("0s"), &MobileUser::FoundObject, mobileUser, *newObject);
    }
  }

//  Simulator::Schedule(Seconds(properties.lifetime), &MobileUser::ExpireContent, this, newObject);

  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " produced object " << *newObject << " # Popularity: " << p.popularity << " # Size: " << p.size << " # Consumers: " << consumers << "/" << users.size());

  return *newObject;

}

// STRATEGY ACTIONS

void
MobileUser::ReplicateContent(Name object)
{
  ProbeVicinity(object);

  Simulator::Schedule(m_vicinityTimer, &MobileUser::PushContent, this, object);
}

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
  vicinity->setNodeId(GetNode()->GetId());
  
  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " sent vicinity probing packet " << vicinity->getName() << " with scope " << vicinity->getScope());
  
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
    m_movingReplicating = true;
    return;
  }

  if (m_vicinity.size() > 0) {
    //ranked 
    if (!m_placementPolicy)
      PushToSelectedDevices(objectName);
    //random
    else
      PushToRandomDevices(objectName);
  }
}

/**
 * Sort the vicinity based on device availability. 
 */
void
MobileUser::PushToSelectedDevices(Name objectName)
{
  // figure out if object is underprovisioned
  objectProperties properties = m_catalog->getObjectProperties(objectName);
  if (m_sessionPeriod + m_movementPeriod == Time("0s"))
    m_userAvailability = 0;
  else
    m_userAvailability = m_sessionPeriod / (m_sessionPeriod + m_movementPeriod);

  // if underprovisioned, send at least 1 copy
  // otherwise, do not send to avoid wasting resources
  if (m_userAvailability > properties.availability)
    return;

  // discover how many devices are in fact interested
  // and map interested consumers to location
  map<uint32_t, uint32_t> consumerDistribution;

  m_consumers.clear();
  for (uint32_t i = 0; i < m_vicinity.size(); i++)
  {
    if (m_vicinity[i].GetInterested())
    {
      m_consumers.push_back(m_vicinity[i]);
      uint32_t location = m_vicinity[i].GetLocation();
      if (consumerDistribution.count(location) == 0)
        consumerDistribution[location] = 0;
      consumerDistribution[location]++;
    }
  }

  if (m_consumers.size() == 0)
    return;

  //sort consumers in the vicinity
  for (uint32_t i = 0; i < m_consumers.size()-1; i++)
  {
    for (uint32_t j = i+1; j < m_consumers.size(); j++)
    {
      if (consumerDistribution[m_consumers[j].GetLocation()] >= consumerDistribution[m_consumers[i].GetLocation()]
            && m_consumers[j].GetAvailability() > m_consumers[i].GetAvailability())
      {
         UserInformation temp = m_consumers[i];
         m_consumers[i] = m_consumers[j];
         m_consumers[j] = temp;
      }
    }
  }

  for (uint32_t i = 0; i < m_consumers.size(); i++)
    NS_LOG_DEBUG("Node " << m_consumers[i].GetNodeId() << " has location/availability/interest " << m_consumers[i].GetLocation() << "/" <<  m_consumers[i].GetAvailability() << "/" << m_consumers[i].GetInterested());

  // calculate the percentage of interested consumers
  double interest = (double) m_consumers.size() / (double) m_vicinity.size();
  uint32_t popularLocation = m_consumers.front().GetLocation();

  // select the device in the popular area with the closest availability to meet the requirements
  double requiredAvailability = 1 - ((1-properties.availability) / m_userAvailability);

  UserInformation provider = SelectBestDevice(popularLocation, true, requiredAvailability);
  HintContent(provider.GetNodeId(), objectName);

  // check if requirements are met and object is unpopular, then send another
  double providersAvailability = 1 - ((1-m_userAvailability) * (1-provider.GetAvailability()));
  if (interest < 0.1 && properties.availability > providersAvailability)
  {
    // select the device not in the popular area with the closest availability to meet the requirements
    requiredAvailability = 1 - ((1-properties.availability) / providersAvailability);

    provider = SelectBestDevice(popularLocation, false, requiredAvailability);
    HintContent(provider.GetNodeId(), objectName);
  }
  
}

/**
 * Select a random device from the vicinity.
 */
void
MobileUser::PushToRandomDevices(Name objectName)
{
  // figure out if object is underprovisioned
  objectProperties properties = m_catalog->getObjectProperties(objectName);
  if (m_sessionPeriod + m_movementPeriod == Time("0s"))
    m_userAvailability = 0;
  else
    m_userAvailability = m_sessionPeriod / (m_sessionPeriod + m_movementPeriod);

  // if underprovisioned, send only 1 copy, we dont have extra info in the random scenario
  // otherwise, do not send to avoid wasting resources
  if (properties.availability > m_userAvailability)
    return;

  m_consumers.clear();
  for (uint32_t i = 0; i < m_vicinity.size(); i++)
  {
    if (m_vicinity[i].GetInterested())
    {
      m_consumers.push_back(m_vicinity[i]);
    }
  }

  UserInformation provider = m_consumers.front();
  HintContent(provider.GetNodeId(), objectName);
  
}

UserInformation
MobileUser::SelectBestDevice(uint32_t location, bool include, double availability)
{
  UserInformation selection = m_consumers.front();

  // find a device in the given location
  if (include)
  {
    for (uint32_t i = 0; i < m_consumers.size(); i++)
    {
      if (m_consumers[i].GetLocation() == location)
      {
        // since it is ordered, the next one will always have a closer availability to the requirement
        // until it is negative (does not meet). Then we return it.
        if (m_consumers[i].GetAvailability() < availability)
          return selection;
        selection = m_consumers[i];
      }
    }
  }
  // find a device not in the location
  else
  {
    for (uint32_t i = 0; i < m_consumers.size(); i++)
    {
      if (m_consumers[i].GetLocation() != location)
      {
        if (m_consumers[i].GetAvailability() < availability)
        {
          // the first device may not meet the requirements. Then, send it anyway instead of the first overall
          if (selection == m_consumers.front())
            selection = m_consumers[i];
          return selection;
        }
        selection = m_consumers[i];
      }
    }
  }
  return selection;
}

/**
 * Send (push) the content to others.
 * We implement it as a hint instead of unsolicited data.
 */
void
MobileUser::HintContent(int deviceId, Name objectName)
{
  // Create the vicinity packet
  Name hintName = Name("/hint");
  hintName.append(objectName);

  shared_ptr<Interest> hint = make_shared<Interest>();
  hint->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  hint->setName(hintName);
  time::milliseconds hintLifeTime(m_hintTimer.GetMilliSeconds());
  hint->setInterestLifetime(hintLifeTime);
  hint->setScope(m_vicinitySize);
  
  hint->setNodeId(deviceId);
  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " hinted content " << hint->getName() << " for device " << hint->getNodeId() << " in " << hint->getScope() << " hops");

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

  m_movingRequesting = false;
  m_movingProducing = false;
  m_movingReplicating = false;

  m_sessionPeriod += (Simulator::Now() - m_lastMovementEvent);

  vector<Ptr<Node>> routers = m_catalog->getRouters();

  ndn::LinkControlHelper::FailLink(this->GetNode(), routers[m_position.x]);

  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " started movement");
}

/**
 *
 */
void
MobileUser::Session(Ptr<const MobilityModel> model)
{
  m_moving = false;
 
  m_movementPeriod += (Simulator::Now() - m_lastMovementEvent);

  m_position = model->GetPosition();

  vector<Ptr<Node>> routers = m_catalog->getRouters();
 
  ndn::LinkControlHelper::UpLink(this->GetNode(), routers[m_position.x]);
  NS_LOG_DEBUG("Node " << GetNode()->GetId() << " started session at router " << m_position.x);

  if (m_movingRequesting)
  {
    Simulator::ScheduleNow(&MobileUser::ScheduleNextInterestPacket, this);
  }

  if (m_chunkRequestQueue.size() + m_retxChunksQueue.size() > 0)
  {
    Simulator::ScheduleNow(&MobileUser::ScheduleNextInterestPacket, this);
  }

  if (m_movingProducing)
  {
    Simulator::ScheduleNow(&MobileUser::ProduceContent, this);
  }

  if (m_movingReplicating)
  {
    Simulator::ScheduleNow(&MobileUser::ReplicateContent, this, m_lastProducedObject);
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
  }
  m_lastMovementEvent = Simulator::Now();

  ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::PrintFIBs();
}


/**
 * @brief Constructor for UserInformation
 */

UserInformation::UserInformation(uint32_t nodeId, uint32_t location, uint32_t availability, bool interested)
{
  m_nodeId = nodeId;
  m_location = location;
  m_availability = availability;
  m_interested = interested;
}

} // namespace ndn
} // namespace ns3


