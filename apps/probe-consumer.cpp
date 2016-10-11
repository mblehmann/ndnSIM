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
 **/

#include "probe-consumer.hpp"
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

#include "utils/ndn-ns3-packet-tag.hpp"
#include "model/ndn-app-face.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include <boost/lexical_cast.hpp>
#include <boost/ref.hpp>

using namespace std;

NS_LOG_COMPONENT_DEFINE("ndn.ProbeConsumer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(ProbeConsumer);

TypeId
ProbeConsumer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::ProbeConsumer")
      .SetGroupName("Ndn")
      .SetParent<App>()

      .AddConstructor<ProbeConsumer>()

      .AddAttribute("Frequency", "Frequency of interest packets",
                    DoubleValue(1.0), MakeDoubleAccessor(&ProbeConsumer::m_frequency),
                    MakeDoubleChecker<double>())

      .AddAttribute("Prefix", "Name of the Interest",
                    StringValue("/prod"), MakeNameAccessor(&ProbeConsumer::m_interestName),
                    MakeNameChecker())

      .AddAttribute("Objects", "Number of Objects",
                    UintegerValue(100), MakeUintegerAccessor(&ProbeConsumer::m_objects),
                    MakeUintegerChecker<uint32_t>())

      .AddTraceSource("PathStretch",
                      "Path stretch of requests",
                      MakeTraceSourceAccessor(&ProbeConsumer::m_pathStretch),
                      "ns3::ndn::ProbeConsumer::PathStretchCallback");

  return tid;
}

ProbeConsumer::ProbeConsumer()
  : m_rand(CreateObject<UniformRandomVariable>())
{
  NS_LOG_FUNCTION_NOARGS();
}

// Application Methods
void
ProbeConsumer::StartApplication() // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS();

  // do base stuff
  App::StartApplication();

  m_first = true;
  ScheduleNextPacket();
}

void
ProbeConsumer::StopApplication() // Called at time specified by Stop
{
  NS_LOG_FUNCTION_NOARGS();

  // cancel periodic packet generation
  Simulator::Cancel(m_sendEvent);

  // cleanup base stuff
  App::StopApplication();
}

void
ProbeConsumer::SendPacket()
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

  uint32_t objectID = m_rand->GetValue(0, m_objects);
  shared_ptr<Name> objectName = make_shared<Name>(m_interestName);
  objectName->append("/obj" + to_string(objectID));

  NS_LOG_DEBUG("Node" << GetNode()->GetId() << " sending interest " << objectName->toUri());

  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*objectName);

  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);

  m_request = Simulator::Now();  

  ScheduleNextPacket();
}

void
ProbeConsumer::ScheduleNextPacket()
{
  if (m_first) {
    m_first = false;
    m_sendEvent = Simulator::Schedule(Seconds(0.5), &ProbeConsumer::SendPacket, this);
  } else {
    m_sendEvent = Simulator::Schedule(Minutes(1.0 / m_frequency), &ProbeConsumer::SendPacket, this);
  }
}

///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////

void
ProbeConsumer::OnData(shared_ptr<const Data> data)
{
  if (!m_active)
    return;

  App::OnData(data); // tracing inside

  NS_LOG_FUNCTION_NOARGS();
  NS_LOG_DEBUG("Node" << GetNode()->GetId() << " receiving data " << data->getName());

  //int hopCount = stoi(data->getName().at(-1));
  uint32_t distHA_MP = stoi(data->getName().at(-1).toUri());
  int hopCount;
  uint32_t prodloc;
  uint32_t sp;
  int stretch;

  if (data->getName().size() == 4)
  {
    distHA_MP = stoi(data->getName().at(-1).toUri());
    hopCount = distHA_MP;
    distHA_MP--;
    prodloc = stoi(data->getName().at(-2).toUri());
  }
  else
  {
    distHA_MP = 0;
    hopCount = 0;
    prodloc = stoi(data->getName().at(-1).toUri());
  }

  auto ns3PacketTag = data->getTag<Ns3PacketTag>();
  if (ns3PacketTag != nullptr) { // e.g., packet came from local node's cache
    FwHopCountTag hopCountTag;
    if (ns3PacketTag->getPacket()->PeekPacketTag(hopCountTag)) {
      hopCount += (hopCountTag.Get()-1);
      NS_LOG_DEBUG("Hop count: " << hopCount);
    }
  }

  if (GetNode()->GetId() > prodloc) sp = GetNode()->GetId() - prodloc;
  else sp = prodloc - GetNode()->GetId();
  stretch = hopCount - sp;

  m_pathStretch(this, data->getName(), hopCount, sp, stretch, distHA_MP, data->getName().at(-2).toUri(), Simulator::Now() - m_request); 
  //NS_LOG_INFO("Consumer" << GetNode()->GetId() << " hc: " << hopCount << " stretch: " << stretch << " shortest-path: " << sp << " dist(HA,MP): " << distHA_MP << " producer-location: " << data->getName().at(-2));

}

} // namespace ndn
} // namespace ns3
