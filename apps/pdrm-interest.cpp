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

#include "pdrm-interest.hpp"

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-fw-hop-count-tag.hpp"

#include "helper/ndn-link-control-helper.hpp"
#include "helper/ndn-fib-helper.hpp"

using namespace std;

NS_LOG_COMPONENT_DEFINE("ndn.PDRMInterest");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(PDRMInterest);

TypeId
PDRMInterest::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::PDRMInterest")
      .SetGroupName("Ndn")
      .SetParent<PDRMMobileProducer>()
      .AddConstructor<PDRMInterest>()

      .AddAttribute("HomeAgentPrefix",
                    "Home Agent Prefix",
                    StringValue("/homeagent"),
                    MakeNameAccessor(&PDRMInterest::m_homeAgentPrefix),
                    MakeNameChecker())

      .AddAttribute("RegisterPrefix",
                    "Register prefix",
                    StringValue("/register"),
                    MakeNameAccessor(&PDRMInterest::m_registerPrefix),
                    MakeNameChecker())

      .AddAttribute("UnregisterPrefix",
                    "Unregister prefix",
                    StringValue("/unregister"),
                    MakeNameAccessor(&PDRMInterest::m_unregisterPrefix),
                    MakeNameChecker());

  return tid;
}

PDRMInterest::PDRMInterest()
{
}

void
PDRMInterest::StartApplication() 
{
  PDRMMobileProducer::StartApplication();
  //m_homeAgentPrefix = m_homeAgentPrefix.toUri() + to_string(m_homeNetwork);
  m_homeAgentPrefix = m_homeAgentPrefix.toUri() + to_string(0);
}

void
PDRMInterest::StopApplication()
{
  PDRMMobileProducer::StopApplication();
}

void
PDRMInterest::EndGame()
{
}

void
PDRMInterest::SendInterest(Name object)
{
  if (!m_active)
    return;

  NS_LOG_INFO(object);

  shared_ptr<Name> interestName = make_shared<Name>(object);

  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*interestName);

  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);
}

void
PDRMInterest::RegisterPrefix(Name prefix)
{ 
  NS_LOG_FUNCTION_NOARGS();
  PDRMProvider::UnannouncePrefix(prefix);

  Name announcement = m_homeAgentPrefix.toUri() + m_registerPrefix.toUri() + prefix.toUri();
  SendInterest(announcement);
}

void
PDRMInterest::UnregisterPrefix(Name prefix)
{ 
  NS_LOG_FUNCTION_NOARGS();
  PDRMProvider::AnnouncePrefix(prefix);

  Name unannouncement = m_homeAgentPrefix.toUri() + m_unregisterPrefix.toUri() + prefix.toUri();
  SendInterest(unannouncement);
}

// MOBILITY ACTIONS

void
PDRMInterest::InterestMove(Ptr<const MobilityModel> model)
{
  NS_LOG_FUNCTION_NOARGS();
  // send message to home agent to store interest
  RegisterPrefix(m_producerPrefix);

  Simulator::Schedule(Time("50ms"), &PDRMMobileProducer::Move, this, model);
}

void
PDRMInterest::InterestSession(Ptr<const MobilityModel> model)
{
  Session(model);

  NS_LOG_FUNCTION_NOARGS();
  UnregisterPrefix(m_producerPrefix);

  // send message to home agent to forward interest
}

// This is equal to PDRMMobileProducer, but the Move and Session parts are different
void
PDRMInterest::CourseChange(Ptr<const MobilityModel> model)
{
  if (!m_mobile)
    return;

  NS_LOG_FUNCTION_NOARGS();
  if (m_moving)
    InterestSession(model);
  else
    InterestMove(model);

  NS_LOG_INFO(m_position.x);
}

} // namespace ndn
} // namespace ns3


