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

#include "pdrm-consumer-batch.hpp"

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-fw-hop-count-tag.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.PDRMConsumerBatch");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(PDRMConsumerBatch);

TypeId
PDRMConsumerBatch::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::PDRMConsumerBatch")
      .SetGroupName("Ndn")
      .SetParent<PDRMConsumer>()
      .AddConstructor<PDRMConsumerBatch>()

      // Consumer
      .AddAttribute("BatchSize",
                    "Number of requests per batch",
                    StringValue("1000"),
                    MakeIntegerAccessor(&PDRMConsumerBatch::m_batchSize),
                    MakeIntegerChecker<uint32_t>());

  return tid;
}

PDRMConsumerBatch::PDRMConsumerBatch()
{
}

/**
 * 
 */
void
PDRMConsumerBatch::StartApplication() 
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
    Simulator::Schedule(m_start, &PDRMConsumerBatch::FindObject, this);
    Simulator::Schedule(Time("1ms"), &PDRMConsumer::WarmUp, this);
  }
}

/**
 * 
 */
void
PDRMConsumerBatch::StopApplication()
{
  PDRMConsumer::StopApplication();
}

/**
 * 
 */
void
PDRMConsumerBatch::FindObject()
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
PDRMConsumerBatch::ScheduleNextPacket()
{ 
  NS_LOG_FUNCTION_NOARGS();
  // send 
  if (!m_sendEvent.IsRunning()) {
    m_sendEvent = Simulator::ScheduleNow(&PDRMConsumerBatch::GetNextPacket, this);
  }
}

void
PDRMConsumerBatch::GetNextPacket()
{
  if (!m_active) 
    return;

  NS_LOG_INFO(m_requestsSent << " "  << m_batchSize);
  Name chunk;

  if (m_requestsSent < m_batchSize) {
    chunk = m_chunkRequest.front();
    m_chunkRequest.pop();
    SendPacket(chunk, false);
    m_requestsSent++;
  } else {
    if (m_defaultConsumer) {
      Simulator::Schedule(Seconds(m_requestPeriod->GetValue()), &PDRMConsumerBatch::FindObject, this);
    }
  }
}

} // namespace ndn
} // namespace ns3


