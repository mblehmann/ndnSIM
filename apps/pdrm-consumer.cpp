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

#include "pdrm-consumer.hpp"

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-fw-hop-count-tag.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.PDRMConsumer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(PDRMConsumer);

TypeId
PDRMConsumer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::PDRMConsumer")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<PDRMConsumer>()

      // Consumer
      .AddAttribute("Lambda",
                    "Requests per second",
                    StringValue("1"),
                    MakeDoubleAccessor(&PDRMConsumer::m_lambdaRequests),
                    MakeDoubleChecker<double>())

      .AddAttribute("Start",
                    "Time when the warm-up period ends and the evaluation begins",
                    StringValue("60s"),
                    MakeTimeAccessor(&PDRMConsumer::m_start),
                    MakeTimeChecker())

      .AddAttribute("End",
                    "Time when the evaluation ends",
                    StringValue("120s"),
                    MakeTimeAccessor(&PDRMConsumer::m_end),
                    MakeTimeChecker())

      .AddAttribute("WarmupPeriod",
                    "Warmup period",
                    StringValue("30s"),
                    MakeTimeAccessor(&PDRMConsumer::m_warmupPeriod),
                    MakeTimeChecker())

      .AddAttribute("Lifetime",
                    "Lifetime for interest packet",
                    StringValue("5s"),
                    MakeTimeAccessor(&PDRMConsumer::m_interestLifeTime),
                    MakeTimeChecker())

      .AddAttribute("DefaultConsumer",
                    "Sets whether the consumer requests content by default",
                    BooleanValue(true),
                    MakeBooleanAccessor(&PDRMConsumer::m_defaultConsumer),
                    MakeBooleanChecker())

      // Global
      .AddAttribute("Catalog",
                    "Content catalog",
                    PointerValue(NULL),
                    MakePointerAccessor(&PDRMConsumer::m_catalog),
                    MakePointerChecker<PDRMCatalog>())

      .AddAttribute("Global",
                    "Global variables",
                    PointerValue(NULL),
                    MakePointerAccessor(&PDRMConsumer::m_global),
                    MakePointerChecker<PDRMGlobal>())

      // Tracing
      .AddTraceSource("ChunkRetrievalDelay",
                      "Delay to retrieve a chunk",
                      MakeTraceSourceAccessor(&PDRMConsumer::m_chunkRetrievalDelay),
                      "ns3::ndn::PDRMConsumer::ChunkRetrievalDelayCallback")

      .AddTraceSource("ObjectDownloadTime",
                      "Time to download an object",
                      MakeTraceSourceAccessor(&PDRMConsumer::m_objectDownloadTime),
                      "ns3::ndn::PDRMConsumer::ObjectDownloadTimeCallback")

      .AddTraceSource("ChunkFailedDelay",
                      "Failed to retrieve a chunk",
                      MakeTraceSourceAccessor(&PDRMConsumer::m_chunkFailedDelay),
                      "ns3::ndn::PDRMConsumer::ChunkFailedDelayCallback")

      .AddTraceSource("ObjectFailedDownload",
                      "Failed to download an object",
                      MakeTraceSourceAccessor(&PDRMConsumer::m_objectFailedDownload),
                      "ns3::ndn::PDRMConsumer::ObjectFailedDownloadCallback");

  return tid;
}

PDRMConsumer::PDRMConsumer()
  : m_rand(CreateObject<UniformRandomVariable>())
{
}

/**
 * 
 */
void
PDRMConsumer::StartApplication() 
{
  App::StartApplication();

  m_maxHopCount = 0;
  m_warmup = true;
  m_execution = false;

  m_requestPeriod = CreateObject<ConstantRandomVariable>();
  m_requestPeriod->SetAttribute("Constant", DoubleValue(1.0 / m_lambdaRequests));

  // schedule the endgame
  Simulator::Schedule(m_global->getMaxSimulationTime() - Time("1ms"), &PDRMConsumer::EndGame, this);

  if (m_defaultConsumer) {
    Simulator::Schedule(m_start, &PDRMConsumer::FindObject, this);
    Simulator::Schedule(Time("1ms"), &PDRMConsumer::WarmUp, this);
  }
}

/**
 * 
 */
void
PDRMConsumer::StopApplication()
{
  Simulator::Cancel(m_sendEvent);
  App::StopApplication();
}

void
PDRMConsumer::EndGame()
{
  Name chunk;
  Name object;
  uint32_t seqNumber;

  for (map<Name, Time>::iterator it = m_chunkFirstRequest.begin(); it != m_chunkFirstRequest.end(); ++it)
  {
    chunk = it->first;
    seqNumber = chunk.at(-1).toSequenceNumber();
    m_chunkFailedDelay(this, chunk, seqNumber, Simulator::Now() - m_chunkFirstRequest[chunk],
      Simulator::Now() - m_chunkLastRequest[chunk], m_chunkRequestCount[chunk], m_maxHopCount);
  }
  
  for (map<Name, Time>::iterator it = m_objectStartDownloadTime.begin(); it != m_objectStartDownloadTime.end(); ++it)
  {
    object = it->first;
    m_objectFailedDownload(this, object, Simulator::Now() - m_objectStartDownloadTime[object], m_objectRequests[object]);
  }
}

void
PDRMConsumer::WarmUp()
{
  NS_LOG_FUNCTION_NOARGS();

  if (Simulator::Now() > m_warmupPeriod) {
    m_warmup = false;
    m_execution = true;
    return;
  }

  ContentObject object = m_catalog->getObjectRequest();
  shared_ptr<Name> interestName;

  // For each chunk, append the sequence number and add it to the interest queue
  for (uint32_t i = 0; i < object.size; i++) {
    shared_ptr<Interest> interest = make_shared<Interest>();
    interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));

    Name interestName = object.name;
    interestName.appendSequenceNumber(i);
    interest->setName(interestName);

    interest->setInterestLifetime((time::milliseconds) m_interestLifeTime.GetMilliSeconds());

    m_transmittedInterests(interest, this, m_face);
    m_face->onReceiveInterest(*interest);
  }

  Simulator::Schedule(Time("1s"), &PDRMConsumer::WarmUp, this);
}

/**
 *  
 */
void
PDRMConsumer::OnTimeout(Name chunk)
{
  if (m_warmup) return;

  if (m_retxEvent.count(chunk) == 0)
    return;

  Name object = chunk.getPrefix(2);
  NS_LOG_FUNCTION(chunk);

  m_objectTimeouts[object]++;
  SendPacket(chunk); 
}

/**
 * 
 */
void
PDRMConsumer::OnData(shared_ptr<const Data> data)
{
  if (m_warmup) return;

  App::OnData(data);

  Name chunk = data->getName();
  Name object = chunk.getPrefix(2);
  uint32_t seqNumber = chunk.at(-1).toSequenceNumber();
 
  if (m_chunksMap[object][seqNumber])
    return;
  NS_LOG_FUNCTION_NOARGS();

  // Calculate the hop count
  uint32_t hopCount = 0;
  auto ns3PacketTag = data->getTag<Ns3PacketTag>();
  if (ns3PacketTag != nullptr) { 
    FwHopCountTag hopCountTag;
    if (ns3PacketTag->getPacket()->PeekPacketTag(hopCountTag)) {
      hopCount = hopCountTag.Get();
    }
  }
  m_maxHopCount = max(m_maxHopCount, hopCount);

  m_chunkRetrievalDelay(this, chunk, seqNumber, Simulator::Now() - m_chunkFirstRequest[chunk],
    Simulator::Now() - m_chunkLastRequest[chunk], m_chunkRequestCount[chunk], hopCount);

  m_chunkFirstRequest.erase(chunk);
  m_chunkLastRequest.erase(chunk);
  m_chunkRequestCount.erase(chunk);

  Simulator::Remove(m_retxEvent[chunk]);
  m_retxEvent.erase(chunk);

  //NS_LOG_INFO(object << " " << seqNumber << " " << m_chunksMap[object][seqNumber] << " " << m_chunksDownloaded[object]);

  m_chunksMap[object][seqNumber] = true;
  m_chunksDownloaded[object]++;

  if (m_chunksDownloaded[object] == m_objectSize[object])
    ConcludeObjectDownload(object);
}

/**
 * 
 */
void
PDRMConsumer::FindObject()
{
  if (Simulator::Now() > m_end) {
    m_execution = false;
    return;
  }

  NS_LOG_FUNCTION_NOARGS();
  ContentObject object;
  do {
    object = m_catalog->getObjectRequest();  
  } while (m_objectStartDownloadTime.count(object.name) > 0);
  NS_LOG_INFO(object.name);
  StartObjectDownload(object);
}

/**
 * 
 */
void
PDRMConsumer::FoundObject(Name interestingObject)
{
  ContentObject object = m_catalog->getObject(interestingObject);
  StartObjectDownload(object);
}

/**
 * 
 */
void
PDRMConsumer::StartObjectDownload(ContentObject object)
{
  shared_ptr<Name> interestName;

  // For each chunk, append the sequence number and add it to the interest queue
  for (uint32_t i = 0; i < object.size; i++) {
    interestName = make_shared<Name>(object.name);
    interestName->appendSequenceNumber(i);
    m_chunkRequest.push(*interestName);
  }

  NS_LOG_INFO(object.name);

  m_objectStartDownloadTime[object.name] = Simulator::Now();
  m_objectSize[object.name] = object.size;
  m_objectRequests[object.name] = 0;
  m_objectTimeouts[object.name] = 0;
  m_chunksMap[object.name] = map<uint32_t, bool>();
  m_chunksDownloaded[object.name] = 0;

  ScheduleNextPacket();
}

/**
 * 
 */
void
PDRMConsumer::ScheduleNextPacket()
{
  NS_LOG_FUNCTION_NOARGS();
  // send 
  if (!m_sendEvent.IsRunning()) {
    m_sendEvent = Simulator::ScheduleNow(&PDRMConsumer::GetNextPacket, this);
  }
}

void
PDRMConsumer::GetNextPacket()
{
  if (!m_active) 
    return;

  NS_LOG_FUNCTION_NOARGS();
  Name chunk;

  if (m_chunkRequest.size() > 0) {
    chunk = m_chunkRequest.front();
    m_chunkRequest.pop();
    SendPacket(chunk);
  } else {
    if (m_defaultConsumer) {
      Simulator::Schedule(Seconds(m_requestPeriod->GetValue()), &PDRMConsumer::FindObject, this);
    }
  }
}

/**
 * 
 */
void
PDRMConsumer::SendPacket(Name chunk)
{
  if (!m_active) 
    return;
  NS_LOG_FUNCTION_NOARGS();

  Name object = chunk.getPrefix(2);

  // Create interest packet
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(chunk);
  interest->setInterestLifetime((time::milliseconds) m_interestLifeTime.GetMilliSeconds());

  // Send packet
  WillSendOutInterest(chunk);

  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);

  m_objectRequests[object]++;
  m_retxEvent[chunk] = Simulator::Schedule(m_interestLifeTime, &PDRMConsumer::OnTimeout, this, chunk);

  ScheduleNextPacket();
}

/**
 * 
 */
void
PDRMConsumer::WillSendOutInterest(Name chunk)
{
  if (m_chunkFirstRequest.count(chunk) == 0)
  {
    m_chunkFirstRequest[chunk] = Simulator::Now();
    m_chunkRequestCount[chunk] = 0;
  }

  m_chunkLastRequest[chunk] = Simulator::Now();
  m_chunkRequestCount[chunk]++;
}

/**
 * 
 */
void
PDRMConsumer::ConcludeObjectDownload(Name object)
{
  m_objectDownloadTime(this, object, Simulator::Now() - m_objectStartDownloadTime[object], m_objectRequests[object]);

  NS_LOG_INFO(object);

  m_objectStartDownloadTime.erase(object);
  m_objectSize.erase(object);
  m_objectRequests.erase(object);
  m_objectTimeouts.erase(object);
  m_chunksMap.erase(object);
  m_chunksDownloaded.erase(object);
}

} // namespace ndn
} // namespace ns3



