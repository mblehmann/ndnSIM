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

#include "probe-agent.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "model/ndn-app-face.hpp"
#include "model/ndn-ns3.hpp"
#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include <memory>

NS_LOG_COMPONENT_DEFINE("ndn.ProbeAgent");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(ProbeAgent);

TypeId
ProbeAgent::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::ProbeAgent")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<ProbeAgent>();

  return tid;
}

ProbeAgent::ProbeAgent()
{
  NS_LOG_FUNCTION_NOARGS();
}

// inherited from Application base class.
void
ProbeAgent::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  Register("/global", "/local");
}

void
ProbeAgent::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}

void
ProbeAgent::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // tracing inside

  NS_LOG_FUNCTION(this << interest);

  if (!m_active)
    return;

  Name globalName(interest->getName().getSubName(0, 1));
  uint32_t seq = interest->getName().at(-1).toSequenceNumber();

  NS_LOG_DEBUG(" << received request for " << interest->getName());
  shared_ptr<Name> localName = make_shared<Name>(m_globals[globalName]);
  localName->appendSequenceNumber(seq);
  NS_LOG_DEBUG(" >> forwarding request for " << *localName);

  shared_ptr<Interest> localInterest = make_shared<Interest>();
  localInterest->setNonce(interest->getNonce());
  localInterest->setName(*localName);

  m_transmittedInterests(localInterest, this, m_face);
  m_face->onReceiveInterest(*localInterest);
}

void
ProbeAgent::OnData(shared_ptr<const Data> data)
{
  // dataName.append(m_postfix);
  // dataName.appendVersion();
  if (!m_active)
    return;

  App::OnData(data); // tracing inside

  NS_LOG_FUNCTION(this << data);

  Name localName(data->getName().getSubName(0, 1));
  uint32_t seq = data->getName().at(-1).toSequenceNumber();
  Name globalName = m_locals[localName];
  NS_LOG_DEBUG(" << received data for " << data->getName());

  Name dataName(globalName);
  dataName.appendSequenceNumber(seq);
  // dataName.append(m_postfix);
  // dataName.appendVersion();
  NS_LOG_DEBUG(" >> forwarding data for " << dataName);

  auto localData = make_shared<Data>();
  localData->setName(dataName);
  localData->setFreshnessPeriod(data->getFreshnessPeriod());

  localData->setContent(data->getContent());
  localData->setSignature(data->getSignature());

  // to create real wire encoding
  localData->wireEncode();

  m_transmittedDatas(localData, this, m_face);
  m_face->onReceiveData(*localData);
}
/*
  auto data = make_shared<Data>();
  data->setName(dataName);
  data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));

  data->setContent(make_shared< ::ndn::Buffer>(m_virtualPayloadSize));

  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }

  signature.setInfo(signatureInfo);
  signature.setValue(::ndn::nonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));

  data->setSignature(signature);

  NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());

  // to create real wire encoding
  data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_face->onReceiveData(*data);
}
*/

void
ProbeAgent::Register(Name global, Name local)
{
  m_globals[global] = local;
  m_locals[local] = global;
  FibHelper::AddRoute(GetNode(), global, m_face, 0);
}

void
ProbeAgent::Unregister(Name global)
{
  m_locals.erase(m_globals[global]);
  m_globals.erase(global);
  FibHelper::RemoveRoute(GetNode(), global, m_face);
}

} // namespace ndn
} // namespace ns3
