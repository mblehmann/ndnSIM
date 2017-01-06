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

#include "pdrm-strategy.hpp"

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-fw-hop-count-tag.hpp"

#include "helper/ndn-fib-helper.hpp"

using namespace std;

NS_LOG_COMPONENT_DEFINE("ndn.PDRMStrategy");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(PDRMStrategy);

TypeId
PDRMStrategy::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::PDRMStrategy")
      .SetGroupName("Ndn")
      .SetParent<PDRMMobileProducer>()
      .AddConstructor<PDRMStrategy>()

      // Strategy
      .AddAttribute("VicinityPrefix",
                    "Prefix for the vicinity messages",
                    StringValue("/vicinity"),
                    MakeNameAccessor(&PDRMStrategy::m_vicinityPrefix),
                    MakeNameChecker())

      .AddAttribute("PlacementPolicy",
                    "Placement Policy",
                    UintegerValue(0),
                    MakeUintegerAccessor(&PDRMStrategy::m_placementPolicy),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("VicinitySize",
                    "Vicinity size",
                    UintegerValue(2),
                    MakeUintegerAccessor(&PDRMStrategy::m_vicinitySize),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("VicinityTimer",
                    "Period to discover vicinity",
                    StringValue("2s"),
                    MakeTimeAccessor(&PDRMStrategy::m_vicinityTimer),
                    MakeTimeChecker())

      .AddAttribute("HintPrefix",
                    "Prefix for the hinting messages",
                    StringValue("/hint"),
                    MakeNameAccessor(&PDRMStrategy::m_hintPrefix),
                    MakeNameChecker())
      
      .AddAttribute("HintTimer",
                    "Lifetime of the hint",
                    StringValue("5s"),
                    MakeTimeAccessor(&PDRMStrategy::m_hintTimer),
                    MakeTimeChecker())

      .AddAttribute("LocalProducer",
                    "Sets whether the producer is biased to a location or not",
                    BooleanValue(false),
                    MakeBooleanAccessor(&PDRMStrategy::m_localProducer),
                    MakeBooleanChecker())

      // Tracing
      .AddTraceSource("ReceivedHint",
                      "Hints received by the node",
                      MakeTraceSourceAccessor(&PDRMStrategy::m_receivedHint),
                      "ns3::ndn::PDRMStrategy::ReceivedHintCallback")

      .AddTraceSource("ReceivedVicinityData",
                      "Vicinity data responses received by the node",
                      MakeTraceSourceAccessor(&PDRMStrategy::m_receivedVicinityData),
                      "ns3::ndn::PDRMStrategy::ReceivedVicinityDataCallback")

      .AddTraceSource("ReplicatedContent",
                      "Content replicated by the node",
                      MakeTraceSourceAccessor(&PDRMStrategy::m_replicatedContent),
                      "ns3::ndn::PDRMStrategy::ReplicatedContentCallback")

      .AddTraceSource("ProbedVicinity",
                      "Number of times the vicinity was probed by the node",
                      MakeTraceSourceAccessor(&PDRMStrategy::m_probedVicinity),
                      "ns3::ndn::PDRMStrategy::ProbedVicinityCallback")

      .AddTraceSource("SelectedDevice",
                      "Devices selected to receive an object copy by the node",
                      MakeTraceSourceAccessor(&PDRMStrategy::m_selectedDevice),
                      "ns3::ndn::PDRMStrategy::SelectedDeviceCallback")

      .AddTraceSource("HintedContent",
                      "Hints sent by the node",
                      MakeTraceSourceAccessor(&PDRMStrategy::m_hintedContent),
                      "ns3::ndn::PDRMStrategy::HintedContentCallback");

  return tid;
}

PDRMStrategy::PDRMStrategy()
{
}

void
PDRMStrategy::StartApplication() 
{
  PDRMMobileProducer::StartApplication();

  // add FIB entriy to the application face (so that the application can receive these messages)
  FibHelper::AddRoute(GetNode(), m_hintPrefix, m_face, 0);
  FibHelper::AddRoute(GetNode(), m_vicinityPrefix, m_face, 0);
}

void
PDRMStrategy::StopApplication()
{
  PDRMMobileProducer::StopApplication();
}

void
PDRMStrategy::EndGame()
{
}

void
PDRMStrategy::PopulateCatalog(uint32_t index)
{
  NS_LOG_INFO(index); 

  //get content name
  int objectIndex = m_producedObjects.size();

  shared_ptr<Name> object = make_shared<Name>(m_producerPrefix);
  Name objectName = m_objectPrefix.toUri() + to_string(objectIndex);
  object->append(objectName);

  m_producedObjects.insert(*object);
  m_lastProducedObject = *object;

  //generate content
  m_catalog->addObject(*object, index);

  ContentObject co = m_catalog->getObject(*object);
  uint32_t popularity = m_catalog->getObjectPopularity(*object);
  m_producedObject(this, *object, co.size, co.availability, popularity);

  m_pendingReplication.push(m_lastProducedObject);
  ReplicateContent();
}

void
PDRMStrategy::OnInterest(shared_ptr<const Interest> interest)
{
  if (!m_active)
    return;

//  NS_LOG_FUNCTION_NOARGS();
  Name object = interest->getName();

  if (m_hintPrefix.isPrefixOf(object))
    OnHint(interest);
  else if (m_vicinityPrefix.isPrefixOf(object))
    OnVicinity(interest);
  else
    PDRMProvider::OnInterest(interest);
}

void
PDRMStrategy::OnHint(shared_ptr<const Interest> interest)
{ 
  NS_LOG_FUNCTION_NOARGS();
  App::OnInterest(interest);
  
  PDRMStrategySelectors incomingSelectors = interest->getPDRMStrategySelectors();
  
  // check if the node is the target
  if (incomingSelectors.getNodeId() == GetNode()->GetId())
  { 
    Name object = interest->getName().getSubName(1, 2);

    m_receivedHint(this, object, true);    
    StoreObject(object);
    Simulator::ScheduleNow(&PDRMConsumer::FoundObject, this, object);
  }
}

void
PDRMStrategy::OnVicinity(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest);
  NS_LOG_INFO(interest->getName());

  Name object = interest->getName().getSubName(1, 2);

  if (m_localProducer) {
    NS_LOG_INFO(m_catalog->getLocalityRequestProbability(object, m_homeNetwork));
    if (m_rand->GetValue(0, 1) > m_catalog->getLocalityRequestProbability(object, m_homeNetwork))
      return;
  }
  else {
    NS_LOG_INFO(m_catalog->getRequestProbability(object));
    if (m_rand->GetValue(0, 1) > m_catalog->getRequestProbability(object))
      return;
  }

  PDRMStrategySelectors incomingSelectors = interest->getPDRMStrategySelectors();
 
  int32_t nodeId = GetNode()->GetId();
  int32_t homeNetwork = m_homeNetwork;
  pair<int32_t, double> preferredLocation = GetPreferredLocation();
  int32_t currentPosition = m_position.x;
  double availability = m_sessionPeriod.GetSeconds() / (m_sessionPeriod.GetSeconds() + m_movementPeriod.GetSeconds() + 0.001);
  bool interested = true;
 
  Name dataName = interest->getName();

  // Create the vicinity packet
  shared_ptr<Data> vicinityData = make_shared<Data>();
  vicinityData->setName(dataName);
  vicinityData->setFreshnessPeriod((time::milliseconds) m_freshness.GetMilliSeconds());

  // Create an empty strategy selector for the Vicinity Data packet 
  PDRMStrategySelectors responseSelectors = PDRMStrategySelectors();

  responseSelectors.setNodeId(nodeId);
  responseSelectors.setHomeNetwork(homeNetwork);
  responseSelectors.setPreferredLocation(preferredLocation.first);
  responseSelectors.setTimeSpentAtPreferredLocation(preferredLocation.second);
  responseSelectors.setCurrentPosition(currentPosition);
  responseSelectors.setAvailability(availability);
  responseSelectors.setInterest(interested);

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

void
PDRMStrategy::OnData(shared_ptr<const Data> data)
{
//  NS_LOG_FUNCTION_NOARGS();
  App::OnData(data);
  Name object = data->getName();

  if (m_vicinityPrefix.isPrefixOf(object))
  {
    App::OnData(data);
    OnVicinityData(data);
  }
  else
  {
    Name chunk = data->getName();
    Name object = chunk.getPrefix(2);
    uint32_t seqNumber = chunk.at(-1).toSequenceNumber();

    if (m_chunksMap[object][seqNumber])
      return;

    // Calculate the hop count
    int hopCount = 0;
    auto ns3PacketTag = data->getTag<Ns3PacketTag>();
    if (ns3PacketTag != nullptr) {
      FwHopCountTag hopCountTag;
      if (ns3PacketTag->getPacket()->PeekPacketTag(hopCountTag)) {
        hopCount = hopCountTag.Get();
      }
    }

    m_chunkRetrievalDelay(this, chunk, seqNumber, Simulator::Now() - m_chunkFirstRequest[chunk],
      Simulator::Now() - m_chunkLastRequest[chunk], m_chunkRequestCount[chunk], hopCount);

    m_chunkFirstRequest.erase(chunk);
    m_chunkLastRequest.erase(chunk);
    m_chunkRequestCount.erase(chunk);
    m_retxEvent.erase(chunk);

    m_chunksMap[object][seqNumber] = true;
    m_chunksDownloaded[object]++;

    if (m_chunksDownloaded[object] == m_objectSize[object])
      ConcludeObjectDownload(object);
  }
}

void
PDRMStrategy::OnVicinityData(shared_ptr<const Data> data)
{
  NS_LOG_INFO(data->getName());
  App::OnData(data);
  Block block = data->getContent();
  block.parse();
  PDRMStrategySelectors incomingSelectors = PDRMStrategySelectors(*block.elements_begin());

  Name object = data->getName().getSubName(1, 2);
  m_receivedVicinityData(this, object, incomingSelectors.getNodeId(), incomingSelectors.getCurrentPosition(), incomingSelectors.getAvailability(), incomingSelectors.getInterest());
  m_vicinity[object].push_back(incomingSelectors);
}

// CONSUMER ACTIONS

void
PDRMStrategy::ScheduleNextPacket()
{
//  NS_LOG_FUNCTION_NOARGS();
  if (m_moving)
    return;

  if (!m_sendEvent.IsRunning()) {
    m_sendEvent = Simulator::ScheduleNow(&PDRMConsumer::GetNextPacket, this);
  }
}

void
PDRMStrategy::ConcludeObjectDownload(Name object)
{
  NS_LOG_FUNCTION_NOARGS();
  PDRMProvider::AnnouncePrefix(object);
  PDRMConsumer::ConcludeObjectDownload(object);
}

// PRODUCER ACTIONS

void
PDRMStrategy::ProduceObject()
{
  PDRMProducer::ProduceObject();
  NS_LOG_INFO(m_lastProducedObject);

  m_pendingReplication.push(m_lastProducedObject);
  ReplicateContent();
}

// STRATEGY ACTIONS

void
PDRMStrategy::ReplicateContent()
{
  if (m_pendingReplication.size() == 0 || m_moving)
    return;

  Name object = m_pendingReplication.front();
  NS_LOG_INFO(object);
  m_replicatedContent(this, object);

  ProbeVicinity(object);
  m_pendingReplication.pop();
  Simulator::Schedule(m_vicinityTimer, &PDRMStrategy::PushContent, this, object);
}

void
PDRMStrategy::ProbeVicinity(Name object)
{
  m_vicinity[object].clear();

  // Create the vicinity packet
  Name vicinityName = m_vicinityPrefix.toUri() + object.toUri();
  NS_LOG_INFO(vicinityName);
  m_probedVicinity(this, object, m_vicinitySize);

  shared_ptr<Interest> vicinity = make_shared<Interest>();
  vicinity->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  vicinity->setName(vicinityName);
  vicinity->setInterestLifetime((time::milliseconds) m_vicinityTimer.GetMilliSeconds());
  vicinity->setScope(m_vicinitySize);
  
  // Send the packet
  vicinity->wireEncode();

  m_transmittedInterests(vicinity, this, m_face);
  m_face->onReceiveInterest(*vicinity);
}

void
PDRMStrategy::PushContent(Name object)
{
  if (m_moving)
    return;

  NS_LOG_INFO(object);

  if (m_vicinity[object].size() > 0) {
    PDRMStrategySelectors selectedDevice;
    //random
    if (!m_placementPolicy)
      selectedDevice = SelectRandomDevice(object);
    //ranked 
    else
      selectedDevice = SelectBestDevice(object);

    m_selectedDevice(this, object, false, true, selectedDevice.getNodeId(), m_userAvailability);
    HintContent(selectedDevice.getNodeId(), object);
  } else {
    m_selectedDevice(this, object, false, false, -1, m_userAvailability);
  }


  ReplicateContent();
}

PDRMStrategySelectors
PDRMStrategy::SelectBestDevice(Name object)
{
  NS_LOG_FUNCTION_NOARGS();

  PDRMStrategySelectors selectedDevice = m_vicinity[object].front();
  for (uint32_t i = 1; i < m_vicinity[object].size(); i++) {
    if (m_vicinity[object][i].getAvailability() > selectedDevice.getAvailability()) {
      selectedDevice = m_vicinity[object][i];
    }
  }
 
  return selectedDevice;
}

PDRMStrategySelectors
PDRMStrategy::SelectRandomDevice(Name object)
{
  NS_LOG_FUNCTION_NOARGS();

  uint32_t deviceIndex = m_rand->GetInteger(0, m_vicinity[object].size()-1);
  PDRMStrategySelectors selectedDevice = m_vicinity[object][deviceIndex];
 
  return selectedDevice;
}

void
PDRMStrategy::HintContent(int deviceId, Name object)
{
  // Create the vicinity packet
  Name hintName = m_hintPrefix.toUri() + object.toUri();
  NS_LOG_INFO(hintName);

  shared_ptr<Interest> hint = make_shared<Interest>();
  hint->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  hint->setName(hintName);
  hint->setInterestLifetime((time::milliseconds) m_hintTimer.GetMilliSeconds());
  hint->setScope(m_vicinitySize);
  hint->setNodeId(deviceId);

  m_hintedContent(this, object, deviceId);

  // Send the packet
  hint->wireEncode();

  m_transmittedInterests(hint, this, m_face);
  m_face->onReceiveInterest(*hint);
}

void
PDRMStrategy::Session(Ptr<const MobilityModel> model)
{
  NS_LOG_FUNCTION_NOARGS();
  PDRMMobileProducer::Session(model);

  if (m_pendingReplication.size() > 0)
    Simulator::ScheduleNow(&PDRMStrategy::ReplicateContent, this);

  if (m_chunkRequest.size() > 0)
    Simulator::ScheduleNow(&PDRMStrategy::ScheduleNextPacket, this);

}

// This is equal to PDRMMobileProducer, but the Session part is different
void
PDRMStrategy::CourseChange(Ptr<const MobilityModel> model)
{
  if (!m_mobile)
    return;

  NS_LOG_FUNCTION_NOARGS();
  if (m_moving)
    Session(model);
  else
    Move(model);

  NS_LOG_INFO(m_position.x);
}


} // namespace ndn
} // namespace ns3


