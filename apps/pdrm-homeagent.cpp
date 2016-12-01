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

#include "pdrm-homeagent.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "utils/ndn-ns3-packet-tag.hpp"

#include "model/ndn-app-face.hpp"
#include "model/ndn-ns3.hpp"
#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include <memory>

NS_LOG_COMPONENT_DEFINE("ndn.PDRMHomeAgent");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(PDRMHomeAgent);

TypeId
PDRMHomeAgent::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::PDRMHomeAgent")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<PDRMHomeAgent>()

      .AddAttribute("HomeAgentPrefix",
                    "Home agent prefix",
                    StringValue("/homeagent"),
                    MakeNameAccessor(&PDRMHomeAgent::m_homeAgentPrefix),
                    MakeNameChecker())

      .AddAttribute("RegisterPrefix",
                    "Register prefix",
                    StringValue("/register"),
                    MakeNameAccessor(&PDRMHomeAgent::m_registerPrefix),
                    MakeNameChecker())
      
      .AddAttribute("UnregisterPrefix",
                    "Unregister prefix",
                    StringValue("/unregister"),
                    MakeNameAccessor(&PDRMHomeAgent::m_unregisterPrefix),
                    MakeNameChecker())

      .AddAttribute("Lifetime",
                    "Lifetime for interest packet",
                    StringValue("5s"),
                    MakeTimeAccessor(&PDRMHomeAgent::m_interestLifeTime),
                    MakeTimeChecker()); 

/*
      // Tracing
      .AddTraceSource("ServedData",
                      "Data served by the provider",
                      MakeTraceSourceAccessor(&PDRMHomeAgent::m_servedData),
                      "ns3::ndn::PDRMHomeAgent::ServedDataCallback");
*/
  return tid;
}

PDRMHomeAgent::PDRMHomeAgent()
{
  NS_LOG_FUNCTION_NOARGS();
}

// inherited from Application base class.
void
PDRMHomeAgent::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  m_homeAgentPrefix = m_homeAgentPrefix.toUri() + to_string(GetNode()->GetId());
  m_registerPrefix = m_homeAgentPrefix.toUri() + m_registerPrefix.toUri();
  m_unregisterPrefix = m_homeAgentPrefix.toUri() + m_unregisterPrefix.toUri();

  Register(m_homeAgentPrefix);
  ndn::GlobalRoutingHelper::PrintFIBs();
}

void
PDRMHomeAgent::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}

void
PDRMHomeAgent::OnTimeout(Name chunk)
{
  Name producerPrefix = chunk.getPrefix(1);

  if (m_retxEvent[producerPrefix].count(chunk) == 0)
    return;

  shared_ptr<Interest> interest;
  for (uint32_t i = 0; i < m_storedInterests[producerPrefix].size(); i++)
  {
    interest = make_shared<Interest>(m_storedInterests[producerPrefix][i]);
    if (chunk == interest->getName()) {
      m_storedInterests[producerPrefix].erase(m_storedInterests[producerPrefix].begin() + i);
      NS_LOG_INFO(chunk);
      return;
    }
  }
}

/**
 * Receives an interest.
 * If it is an update request, update the locator for the prefix (/update/prefix/locator)
 * If it is a register request, register the locator for the prefix (/register/prefix/locator)
 * If it is an unregister request, unregister the locator for the prefix (/unregister/prefix)
 * If it is an object request, pre-append the locator and forward it
 */
void
PDRMHomeAgent::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // tracing inside

  if (!m_active)
    return;

  NS_LOG_INFO(interest->getName());

  Name object = interest->getName();
  Name producerPrefix = object.getSubName(2);

  if (m_registerPrefix.isPrefixOf(object))
  {
    producerPrefix = object.getSubName(2);
    Register(producerPrefix);
  }
  else if (m_unregisterPrefix.isPrefixOf(object))
  {
    producerPrefix = object.getSubName(2);
    Unregister(producerPrefix);
  }
  else
  {
    producerPrefix = object.getPrefix(1);
    if (m_unavailableProducers.count(producerPrefix) > 0) { 
      m_storedInterests[producerPrefix].push_back(*interest);
      m_retxEvent[producerPrefix][object] = Simulator::Schedule(m_interestLifeTime, &PDRMHomeAgent::OnTimeout, this, object);
      //m_servedData(this, interest->getName(), "homeagent");
    }
  }
}

void
PDRMHomeAgent::Register(Name producerPrefix)
{
  NS_LOG_INFO(producerPrefix);

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper = m_global->getGlobalRoutingHelper();
  ndnGlobalRoutingHelper.AddOrigin(producerPrefix.toUri(), this->GetNode());

  FibHelper::AddRoute(GetNode(), producerPrefix, m_face, 0);
  ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::PrintFIBs();

  m_unavailableProducers.insert(producerPrefix);
}

void
PDRMHomeAgent::Unregister(Name producerPrefix)
{
  NS_LOG_INFO(producerPrefix << " " << m_storedInterests[producerPrefix].size());

  shared_ptr<Interest> interest;

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper = m_global->getGlobalRoutingHelper();
  ndnGlobalRoutingHelper.RemoveOrigin(producerPrefix.toUri(), GetNode());

  FibHelper::RemoveRoute(GetNode(), producerPrefix, m_face);
  ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::PrintFIBs();

  for (uint32_t i = 0; i < m_storedInterests[producerPrefix].size(); i++)
  {
    interest = make_shared<Interest>(m_storedInterests[producerPrefix][i]);
    m_transmittedInterests(interest, this, m_face);
    m_face->onReceiveInterest(*interest);
    NS_LOG_INFO(interest->getName());
  }

  m_storedInterests.erase(producerPrefix);
  m_unavailableProducers.erase(producerPrefix);
  m_retxEvent.erase(producerPrefix);
}

} // namespace ndn
} // namespace ns3
