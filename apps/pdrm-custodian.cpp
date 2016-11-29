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

#include "pdrm-custodian.hpp"

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-fw-hop-count-tag.hpp"

#include "helper/ndn-fib-helper.hpp"

using namespace std;

NS_LOG_COMPONENT_DEFINE("ndn.PDRMCustodian");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(PDRMCustodian);

TypeId
PDRMCustodian::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::PDRMCustodian")
      .SetGroupName("Ndn")
      .SetParent<PDRMStrategy>()
      .AddConstructor<PDRMCustodian>()

      // Custodian
      .AddAttribute("CustodianPrefix",
                    "Prefix for the custodian",
                    StringValue("/custodian<ID>"),
                    MakeNameAccessor(&PDRMCustodian::m_custodianPrefix),
                    MakeNameChecker())

      .AddAttribute("Custodian",
                    "Sets whether application acts as a custodian or a producer",
                    BooleanValue(false),
                    MakeBooleanAccessor(&PDRMCustodian::m_custodian),
                    MakeBooleanChecker());

  return tid;
}

PDRMCustodian::PDRMCustodian()
{
}

void
PDRMCustodian::StartApplication() 
{
  PDRMMobileProducer::StartApplication();

  if (m_custodian) {
    AnnouncePrefix(m_custodianPrefix);
    m_warmup = false;
    m_execution = true;
  }
}

void
PDRMCustodian::StopApplication()
{
  PDRMMobileProducer::StopApplication();
}

void
PDRMCustodian::EndGame()
{
}

void
PDRMCustodian::OnInterest(shared_ptr<const Interest> interest)
{
  if (!m_active)
    return;

//  NS_LOG_INFO(interest->getName());
  Name object = interest->getName();

  if (object.getSubName(1, 1) == m_hintPrefix)
    OnHint(interest);
  else
    PDRMProvider::OnInterest(interest);
}

void
PDRMCustodian::OnHint(shared_ptr<const Interest> interest)
{
  NS_LOG_INFO(interest->getName());
  App::OnInterest(interest);

  Name object = interest->getName().getSubName(2);

  StoreObject(object);
  Simulator::ScheduleNow(&PDRMConsumer::FoundObject, this, object);
}

// PRODUCER ACTIONS

void
PDRMCustodian::ProduceObject()
{
  PDRMProducer::ProduceObject();
  NS_LOG_INFO(m_lastProducedObject);

  m_pendingReplication.push(m_lastProducedObject);
  ReplicateContent();
}

// CUSTODIAN ACTIONS

void
PDRMCustodian::ReplicateContent()
{
  if (m_pendingReplication.size() == 0)
    return;

  Name object = m_pendingReplication.front();
  NS_LOG_INFO(object);
  PushContent(object);
}

void
PDRMCustodian::PushContent(Name object)
{
  if (m_moving)
    return;

  NS_LOG_INFO(object);
  HintContent(object);

  m_pendingReplication.pop();
  ReplicateContent();
}

void
PDRMCustodian::HintContent(Name object)
{
  // Create the vicinity packet
  // name: /custodian/hint/object
  Name hintName = m_custodianPrefix.toUri() + m_hintPrefix.toUri() + object.toUri();
  NS_LOG_INFO(hintName);

  shared_ptr<Interest> hint = make_shared<Interest>();
  hint->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  hint->setName(hintName);
  hint->setInterestLifetime((time::milliseconds) m_hintTimer.GetMilliSeconds());

  // Send the packet
  hint->wireEncode();

  m_transmittedInterests(hint, this, m_face);
  m_face->onReceiveInterest(*hint);
}

} // namespace ndn
} // namespace ns3


