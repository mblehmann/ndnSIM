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
                    MakeTimeChecker())

      // Global
      .AddAttribute("Catalog",
                    "Content catalog",
                    PointerValue(NULL),
                    MakePointerAccessor(&PDRMHomeAgent::m_catalog),
                    MakePointerChecker<PDRMCatalog>())

      .AddAttribute("Global",
                    "Global variables",
                    PointerValue(NULL),
                    MakePointerAccessor(&PDRMHomeAgent::m_global),
                    MakePointerChecker<PDRMGlobal>())

      // Tracing
      .AddTraceSource("AnnouncedPrefix",
                      "Announcement and Unannoncement of prefixes by the home agent",
                      MakeTraceSourceAccessor(&PDRMHomeAgent::m_announcedPrefix),
                      "ns3::ndn::PDRMHomeAgent::AnnouncedPrefixCallback")

      .AddTraceSource("InterceptedInterest",
                      "Interests intercepted by the home agent",
                      MakeTraceSourceAccessor(&PDRMHomeAgent::m_interceptedInterest),
                      "ns3::ndn::PDRMHomeAgent::InterceptedInterestCallback");

  return tid;
}

PDRMHomeAgent::PDRMHomeAgent()
  : m_rand(CreateObject<UniformRandomVariable>())
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
PDRMHomeAgent::OnTimeout(Name object)
{
  Name producerPrefix = object.getPrefix(1);

  if (m_retxEvent[producerPrefix].count(object) == 0)
    return;

  shared_ptr<Interest> interest;
  for (uint32_t i = 0; i < m_storedObjects[producerPrefix].size(); i++)
  {
    if (object.toUri() == m_storedObjects[producerPrefix][i].toUri()) {
      m_storedObjects[producerPrefix].erase(m_storedObjects[producerPrefix].begin() + i);
      NS_LOG_INFO(object);

      m_interceptedInterest(this, object, false, true, false);
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

  Name chunk = interest->getName();
  Name producerPrefix = chunk.getSubName(2);

  if (m_registerPrefix.isPrefixOf(chunk))
  {
    producerPrefix = chunk.getSubName(2);
    Register(producerPrefix);
  }
  else if (m_unregisterPrefix.isPrefixOf(chunk))
  {
    producerPrefix = chunk.getSubName(2);
    Unregister(producerPrefix);
  }
  else
  {
    producerPrefix = chunk.getPrefix(1);
    if (m_unavailableProducers.count(producerPrefix) > 0) { 
      Name object = chunk.getPrefix(2);
      for (uint32_t i = 0; i < m_storedObjects[producerPrefix].size(); i++) {
        if (object.toUri() == m_storedObjects[producerPrefix][i].toUri()) {
          Simulator::Remove(m_retxEvent[producerPrefix][object]);
          m_retxEvent[producerPrefix][object] = Simulator::Schedule(m_interestLifeTime, &PDRMHomeAgent::OnTimeout, this, object);
          return;
        }
      }
      NS_LOG_INFO(object);
      m_interceptedInterest(this, object, true, false, false);

      m_storedObjects[producerPrefix].push_back(object);
      m_retxEvent[producerPrefix][object] = Simulator::Schedule(m_interestLifeTime, &PDRMHomeAgent::OnTimeout, this, object);
    }
  }
}

void
PDRMHomeAgent::Register(Name producerPrefix)
{
  NS_LOG_INFO(producerPrefix);
  m_announcedPrefix(this, producerPrefix, true);

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
  NS_LOG_INFO(producerPrefix << " " << m_storedObjects[producerPrefix].size());
  m_announcedPrefix(this, producerPrefix, false);

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper = m_global->getGlobalRoutingHelper();
  ndnGlobalRoutingHelper.RemoveOrigin(producerPrefix.toUri(), GetNode());

  FibHelper::RemoveRoute(GetNode(), producerPrefix, m_face);
  ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::PrintFIBs();

  for (uint32_t i = 0; i < m_storedObjects[producerPrefix].size(); i++)
  {
    ContentObject object = m_catalog->getObject(m_storedObjects[producerPrefix][i]);

    for (uint32_t j = 0; j < object.size; j++) {
      shared_ptr<Interest> interest = make_shared<Interest>();
      interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));

      Name interestName = object.name;
      interestName.appendSequenceNumber(j);
      interest->setName(interestName);
      interest->setInterestLifetime((time::milliseconds) m_interestLifeTime.GetMilliSeconds());

      m_transmittedInterests(interest, this, m_face);
      m_face->onReceiveInterest(*interest);
    }

    m_interceptedInterest(this, object.name, false, false, true);
    NS_LOG_INFO(m_storedObjects[producerPrefix][i]);
  }

  m_storedObjects.erase(producerPrefix);
  m_unavailableProducers.erase(producerPrefix);
  m_retxEvent.erase(producerPrefix);
}

} // namespace ndn
} // namespace ns3
