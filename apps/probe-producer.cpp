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

using namespace std;

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

      .AddAttribute("Routers", "Routers in the network",
                    PointerValue(NULL), MakePointerAccessor(&ProbeProducer::m_routers),
                    MakePointerChecker<Catalog>())

      .AddAttribute("Prefix", "Prefix, for which producer has the data",
                    StringValue("/prod"), MakeNameAccessor(&ProbeProducer::m_prefix),
                    MakeNameChecker())

      .AddAttribute("HomeAgent", "Does the producer use a home agent?",
                    BooleanValue("true"), MakeBooleanAccessor(&ProbeProducer::m_homeAgent),
                    MakeBooleanChecker())

      .AddAttribute("Home", "Home location of the producer",
                    VectorValue(Vector(0, 0, 0)), MakeVectorAccessor(&ProbeProducer::m_home),
                    MakeVectorChecker())

      .AddAttribute("PayloadSize", "Virtual payload size for Content packets",
                    UintegerValue(1024), MakeUintegerAccessor(&ProbeProducer::m_virtualPayloadSize),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("Freshness", "Freshness of data packets, if 0, then unlimited freshness",
                    TimeValue(Seconds(1)), MakeTimeAccessor(&ProbeProducer::m_freshness),
                    MakeTimeChecker())

      .AddAttribute("Signature", "Fake signature, 0 valid signature (default), other values application-specific",
                    UintegerValue(0), MakeUintegerAccessor(&ProbeProducer::m_signature),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("KeyLocator", "Name to be used for key locator.  If root, then key locator is not used",
                    NameValue(), MakeNameAccessor(&ProbeProducer::m_keyLocator),
                    MakeNameChecker())

      .AddTraceSource("FIBChanges",
                      "Number of FIB changes caused by movement",
                      MakeTraceSourceAccessor(&ProbeProducer::m_FIBChanges),
                      "ns3::ndn::ProbeProducer::FIBChangesCallback")

      .AddTraceSource("ServedData",
                      "Data served by the provider",
                      MakeTraceSourceAccessor(&ProbeProducer::m_servedData),
                      "ns3::ndn::ProbeProducer::ServedDataCallback");

  return tid;
}

ProbeProducer::ProbeProducer()
  : m_rand(CreateObject<UniformRandomVariable>())
{
  NS_LOG_FUNCTION_NOARGS();
}

// inherited from Application base class.
void
ProbeProducer::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  Ptr<MobilityModel> mob = GetNode()->GetObject<MobilityModel>();
  mob->TraceConnectWithoutContext("CourseChange", MakeCallback(&ProbeProducer::CourseChange, this));
  m_location = m_home;
  if (m_homeAgent) {
    Register();
  } else {
    FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
  }
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

  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

  Name dataName(interest->getName());
  dataName.append(to_string((int) m_location.x));

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

  // to create real wire encoding
  data->wireEncode();

  m_transmittedDatas(data, this, m_face);
  m_face->onReceiveData(*data);

  //m_servedData(this, interest->getName());
}

void
ProbeProducer::SendInterest(string name)
{
  if (!m_active)
    return;
  
  NS_LOG_FUNCTION_NOARGS();
  
  shared_ptr<Name> objectName = make_shared<Name>(name);
  
  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*objectName);
  
  m_transmittedInterests(interest, this, m_face);
  m_face->onReceiveInterest(*interest);

  NS_LOG_DEBUG("Node" << GetNode()->GetId() << " sending interest " << objectName->toUri());
}

void
ProbeProducer::Register()
{
  NS_LOG_FUNCTION_NOARGS();
  FibHelper::AddRoute(GetNode(), "/loc" + to_string((int) m_home.x) + m_prefix.toUri(), m_face, 0);

  SendInterest("/ha0/register" + m_prefix.toUri() + "/loc" + to_string((int) m_home.x));
}

void
ProbeProducer::UpdateLocation()
{
  NS_LOG_FUNCTION_NOARGS();
  FibHelper::AddRoute(GetNode(), "/loc" + to_string((int) m_location.x) + m_prefix.toUri(), m_face, 0);
  SendInterest("/ha0/update" + m_prefix.toUri() + "/loc" + to_string((int) m_location.x));
}

void
ProbeProducer::CourseChange(Ptr<const MobilityModel> model)
{
  NS_LOG_FUNCTION_NOARGS();
  Ptr<Node> router = m_routers->getRouters()[(int) m_location.x];
  ndn::LinkControlHelper::FailLink(router, GetNode());
  if (m_homeAgent) {
    FibHelper::RemoveRoute(GetNode(), "/loc" + to_string((int) m_location.x) + m_prefix.toUri(), m_face);
  }

  m_location = model->GetPosition();
  router = m_routers->getRouters()[(int) m_location.x];
  ndn::LinkControlHelper::UpLink(router, GetNode());
  if (m_homeAgent) {
    FibHelper::AddRoute(GetNode(), "/loc" + to_string((int) m_location.x) + m_prefix.toUri(), m_face, 0);
  }

  NS_LOG_INFO("Producer" << GetNode()->GetId() << " new position " << m_location.x);

  uint32_t changes = ndn::GlobalRoutingHelper::CalculateRoutes();
  m_FIBChanges(this, m_prefix, changes);
  ndn::GlobalRoutingHelper::PrintFIBs();

  if (m_homeAgent) {
    SendInterest("/ha0/update" + m_prefix.toUri() + "/loc" + to_string((int) m_location.x));
  }
}

} // namespace ndn
} // namespace ns3
