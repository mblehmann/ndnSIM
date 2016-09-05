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

#include "probe-producer.hpp"
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

NS_LOG_COMPONENT_DEFINE("ndn.ProbeProducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(ProbeProducer);

TypeId
ProbeProducer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::ProbeProducer")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<ProbeProducer>()

      .AddAttribute("Catalog", "Content catalog",
                    PointerValue(NULL),
                    MakePointerAccessor(&ProbeProducer::m_catalog),
                    MakePointerChecker<Catalog>())

      .AddAttribute("Prefix", "Prefix, for which producer has the data", StringValue("/"),
                    MakeNameAccessor(&ProbeProducer::m_prefix), MakeNameChecker())
      .AddAttribute("PayloadSize", "Virtual payload size for Content packets", UintegerValue(1024),
                    MakeUintegerAccessor(&ProbeProducer::m_virtualPayloadSize),
                    MakeUintegerChecker<uint32_t>())
      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                    TimeValue(Seconds(0)), MakeTimeAccessor(&ProbeProducer::m_freshness),
                    MakeTimeChecker())
      .AddAttribute(
         "Signature",
         "Fake signature, 0 valid signature (default), other values application-specific",
         UintegerValue(0), MakeUintegerAccessor(&ProbeProducer::m_signature),
         MakeUintegerChecker<uint32_t>())
      .AddAttribute("KeyLocator",
                    "Name to be used for key locator.  If root, then key locator is not used",
                    NameValue(), MakeNameAccessor(&ProbeProducer::m_keyLocator), MakeNameChecker());
  return tid;
}

ProbeProducer::ProbeProducer()
{
  NS_LOG_FUNCTION_NOARGS();
}

// inherited from Application base class.
void
ProbeProducer::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);

  Ptr<MobilityModel> mob = this->GetNode()->GetObject<MobilityModel>();
  mob->TraceConnectWithoutContext("CourseChange", MakeCallback(&ProbeProducer::CourseChange, this));
}

void
ProbeProducer::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}

void
ProbeProducer::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // tracing inside

  NS_LOG_FUNCTION(this << interest);

  if (!m_active)
    return;

  Name dataName(interest->getName());
  // dataName.append(m_postfix);
  // dataName.appendVersion();

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

void
ProbeProducer::CourseChange(Ptr<const MobilityModel> model)
{

  vector<Ptr<Node>> routers = m_catalog->getRouters();

  for (uint32_t i = 0; i < routers.size(); i++) {
    cout << "Router " << i << " " <<  endl;
    Ptr<Node> currentRouter = routers[i];
    ndn::LinkControlHelper::FailLink(currentRouter, this->GetNode());
  }

  Vector pos = model->GetPosition();


  for (uint32_t i = 0; i < routers.size(); i++) {
    Ptr<Node> currentRouter = routers[i];
    
    if (currentRouter->GetId() == pos.x) {
      ndn::LinkControlHelper::UpLink(currentRouter, this->GetNode());
    }
  }

  ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::PrintFIBs();

  NS_LOG_DEBUG("Node " << GetNode()->GetId() << ": " << pos.x << ", " << pos.y);
}

} // namespace ndn
} // namespace ns3
