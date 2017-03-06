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

#include "pdrm-consumer-streaming.hpp"

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-fw-hop-count-tag.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.PDRMConsumerStreaming");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(PDRMConsumerStreaming);

TypeId
PDRMConsumerStreaming::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::PDRMConsumerStreaming")
      .SetGroupName("Ndn")
      .SetParent<PDRMConsumer>()
      .AddConstructor<PDRMConsumerStreaming>()

      // Consumer
      .AddAttribute("BatchSize",
                    "Interests per request",
                    StringValue("20"),
                    MakeIntegerAccessor(&PDRMConsumerStreaming::m_batchSize),
                    MakeIntegerChecker<uint32_t>());

  return tid;
}

PDRMConsumerStreaming::PDRMConsumerStreaming()
{
}

/**
 * 
 */
void
PDRMConsumerStreaming::StartApplication() 
{
  App::StartApplication();

  m_maxHopCount = 0;
  m_warmup = true;
  m_execution = false;

  m_requestsSent = 0;

  m_requestPeriod = CreateObject<ConstantRandomVariable>();
  m_requestPeriod->SetAttribute("Constant", DoubleValue(1.0 / m_lambdaRequests));

  // schedule the endgame
  Simulator::Schedule(m_global->getMaxSimulationTime() - Time("1ms"), &PDRMConsumerStreaming::EndGame, this);

  if (m_defaultConsumer) {
    Simulator::Schedule(m_start, &PDRMConsumerStreaming::FindObject, this);
    Simulator::Schedule(Time("1ms"), &PDRMConsumer::WarmUp, this);
  }
}

/**
 * 
 */
void
PDRMConsumerStreaming::StopApplication()
{
  Simulator::Cancel(m_sendEvent);
  App::StopApplication();
}

void
PDRMConsumerStreaming::EndGame()
{
}

/**
 *  
 */
void
PDRMConsumerStreaming::OnTimeout(Name chunk)
{
  if (m_warmup) return;

  if (m_retxEvent.count(chunk) == 0)
    return;

  Name object = chunk.getPrefix(2);
  NS_LOG_FUNCTION(chunk);

  uint32_t seqNumber = chunk.at(-1).toSequenceNumber();

  m_chunkFailedDelay(this, chunk, seqNumber, Simulator::Now() - m_chunkFirstRequest[chunk],
    Simulator::Now() - m_chunkLastRequest[chunk], m_chunkRequestCount[chunk], m_maxHopCount);
  
  m_objectTimeouts[object]++;
}

/**
 * 
 */
void
PDRMConsumerStreaming::OnData(shared_ptr<const Data> data)
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

  if (seqNumber+1 == m_objectSize[object])
    ConcludeObjectDownload(object);
}

/**
 * 
 */
void
PDRMConsumerStreaming::FindObject()
{
  if (Simulator::Now() > m_end) {
    m_execution = false;
    return;
  }

  m_requestsSent = 0;

  if (m_chunkRequest.size() > 0) {
    ScheduleNextPacket();
  } else {
    NS_LOG_FUNCTION_NOARGS();
    ContentObject object;
    do {
      object = m_catalog->getObjectRequest();
    } while (m_objectStartDownloadTime.count(object.name) > 0);
    NS_LOG_INFO(object.name);
    StartObjectDownload(object);
  }
}

/**
 * 
 */
void
PDRMConsumerStreaming::StartObjectDownload(ContentObject object)
{
  shared_ptr<Name> interestName;

  // For each chunk, append the sequence number and add it to the interest queue
  for (uint32_t i = 0; i < object.size; i++) {
    interestName = make_shared<Name>(object.name);
    interestName->appendSequenceNumber(i);
    m_chunkRequest.push(*interestName);
  }

  NS_LOG_INFO(object.name << " " << object.locality << " @ " << m_position);

  m_objectStartDownloadTime[object.name] = Simulator::Now();
  m_objectSize[object.name] = object.size;
  m_objectTimeouts[object.name] = 0;
  m_chunksMap[object.name] = map<uint32_t, bool>();

  ScheduleNextPacket();
}

/**
 * 
 */
void
PDRMConsumerStreaming::ScheduleNextPacket()
{
  NS_LOG_FUNCTION_NOARGS();
  // send 
  if (!m_sendEvent.IsRunning()) {
    m_sendEvent = Simulator::ScheduleNow(&PDRMConsumerStreaming::GetNextPacket, this);
  }
}

void
PDRMConsumerStreaming::GetNextPacket()
{
  if (!m_active) 
    return;

  NS_LOG_FUNCTION_NOARGS();
  Name chunk;

  if (m_requestsSent < m_batchSize) {
    chunk = m_chunkRequest.front();
    m_chunkRequest.pop();
    SendPacket(chunk, false);
    m_requestsSent++;
  } else {
    if (m_defaultConsumer) {
      Simulator::Schedule(Seconds(m_requestPeriod->GetValue()), &PDRMConsumerStreaming::FindObject, this);
    }
  }
}

/**
 * 
 */
void
PDRMConsumerStreaming::SendPacket(Name chunk, bool retransmission)
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

  m_retxEvent[chunk] = Simulator::Schedule(m_interestLifeTime, &PDRMConsumerStreaming::OnTimeout, this, chunk);

  ScheduleNextPacket();
}

/**
 * 
 */
void
PDRMConsumerStreaming::WillSendOutInterest(Name chunk)
{
  m_chunkFirstRequest[chunk] = Simulator::Now();
  m_chunkLastRequest[chunk] = Simulator::Now();

  m_chunkRequestCount[chunk] = 1;
}

/**
 * 
 */
void
PDRMConsumerStreaming::ConcludeObjectDownload(Name object)
{
  m_objectDownloadTime(this, object, Simulator::Now() - m_objectStartDownloadTime[object], m_objectTimeouts[object]);

  NS_LOG_INFO(object);

  m_objectStartDownloadTime.erase(object);
  m_objectSize.erase(object);
  m_objectTimeouts.erase(object);
  m_chunksMap.erase(object);
}

} // namespace ndn
} // namespace ns3



