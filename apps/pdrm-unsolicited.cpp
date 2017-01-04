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

#include "pdrm-unsolicited.hpp"

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-fw-hop-count-tag.hpp"

#include "helper/ndn-link-control-helper.hpp"
#include "helper/ndn-fib-helper.hpp"

using namespace std;

NS_LOG_COMPONENT_DEFINE("ndn.PDRMUnsolicited");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(PDRMUnsolicited);

TypeId
PDRMUnsolicited::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::PDRMUnsolicited")
      .SetGroupName("Ndn")
      .SetParent<PDRMMobileProducer>()
      .AddConstructor<PDRMUnsolicited>()

      // Unsolicited
      .AddAttribute("ObjectsToPush",
                    "How many objects to push before movement",
                    UintegerValue(10),
                    MakeUintegerAccessor(&PDRMUnsolicited::m_objectsToPush),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("IdleRequest",
                    "Period to consider a request idle",
                    StringValue("5s"),
                    MakeTimeAccessor(&PDRMUnsolicited::m_idleRequest),
                    MakeTimeChecker())

      // Tracing
      .AddTraceSource("PushedUnsolicitedData",
                      "Data chunks pushed by the producer",
                      MakeTraceSourceAccessor(&PDRMUnsolicited::m_pushedUnsolicitedData),
                      "ns3::ndn::PDRMUnsolicited::PushedUnsolicitedDataCallback")

      .AddTraceSource("PushedUnsolicitedObject",
                      "Objects pushed by the producer",
                      MakeTraceSourceAccessor(&PDRMUnsolicited::m_pushedUnsolicitedObject),
                      "ns3::ndn::PDRMUnsolicited::PushedUnsolicitedObjectCallback");

  return tid;
}

PDRMUnsolicited::PDRMUnsolicited()
{
}

void
PDRMUnsolicited::StartApplication() 
{
  PDRMMobileProducer::StartApplication();
}

void
PDRMUnsolicited::StopApplication()
{
  PDRMMobileProducer::StopApplication();
}

void
PDRMUnsolicited::EndGame()
{
}

void
PDRMUnsolicited::OnTimeoutUnsolicitedData(Name object)
{
  NS_LOG_INFO(object);
  m_pushedUnsolicitedObject(this, object, false, true);

  m_timeoutUnsolicitedData.erase(object);
  for (uint32_t i = 0; i < m_activeRequests.size(); i++)
  {
    if (object.toUri() == m_activeRequests[i].toUri()) {
      m_activeRequests.erase(m_activeRequests.begin() + i);
      NS_LOG_INFO(object);
      break;
    }
  }
}

void
PDRMUnsolicited::OnInterest(shared_ptr<const Interest> interest)
{ 
  App::OnInterest(interest);
  
  if (!m_active)
    return;
  
  Name dataName(interest->getName());
  
  Name producerPrefix = dataName.getPrefix(1);
  Name objectPrefix = dataName.getPrefix(2);
  
  if (m_announcedPrefixes.count(producerPrefix) == 0 && m_announcedPrefixes.count(objectPrefix) == 0)
    return;

  //NS_LOG_INFO(interest->getName());

  /* Update data structures */
  uint32_t seqNumber = dataName.at(-1).toSequenceNumber();

  bool exists = false;
  for (uint32_t i = 0; i < m_activeRequests.size(); i++)
  {
    if (objectPrefix.toUri() == m_activeRequests[i].toUri()) {
      exists = true;
      break;
    }
  }

  if (!exists)
    m_activeRequests.push_back(objectPrefix);

  Simulator::Remove(m_timeoutUnsolicitedData[objectPrefix]);
  m_timeoutUnsolicitedData[objectPrefix] = Simulator::Schedule(m_idleRequest, &PDRMUnsolicited::OnTimeoutUnsolicitedData, this, objectPrefix);

  /* Check if it is still active */
  ContentObject properties = m_catalog->getObject(objectPrefix);
  if (properties.size == seqNumber + 1)
  {
    for (uint32_t i = 0; i < m_activeRequests.size(); i++)
    {
      if (objectPrefix.toUri() == m_activeRequests[i].toUri()) {
        m_activeRequests.erase(m_activeRequests.begin() + i);
        NS_LOG_INFO(objectPrefix);
        break;
      }
    }
    Simulator::Remove(m_timeoutUnsolicitedData[objectPrefix]);
    m_timeoutUnsolicitedData.erase(objectPrefix);
  }

  /* End of changes */

  // Create data packet
  auto data = make_shared<Data>();
  data->setName(dataName);
  data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
  data->setContent(make_shared< ::ndn::Buffer>(m_virtualPayloadSize));
  
  // Add signature
  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));
  
  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }
  
  signature.setInfo(signatureInfo);
  signature.setValue(::ndn::nonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));
  
  data->setSignature(signature);
  
  // logging and stats
  m_servedData(this, interest->getName());
  
  // Send it
  data->wireEncode();
  
  m_transmittedDatas(data, this, m_face);
  m_face->onReceiveData(*data);
}

void
PDRMUnsolicited::PushUnsolicitedData()
{
  NS_LOG_INFO(m_activeRequests.size());

  if (m_activeRequests.size() == 0)
    return;

  shared_ptr<Name> chunk;
  ContentObject object;

  for (uint32_t i = 0; i < m_activeRequests.size(); i++)
  {
    object = m_catalog->getObject(m_activeRequests[i]);
    if (i < m_objectsToPush) {
      m_pushedUnsolicitedObject(this, object.name, true, false);
      NS_LOG_INFO(object.name);
      for (uint32_t i = 0; i < object.size; i++)
      {
        chunk = make_shared<Name>(object.name);
        chunk->appendSequenceNumber(i);
        SendUnsolicitedData(*chunk);
      }
    } else {
      m_pushedUnsolicitedObject(this, object.name, false, false);
    }
  }

  for (map<Name, EventId>::iterator it = m_timeoutUnsolicitedData.begin(); it != m_timeoutUnsolicitedData.end(); ++it)
    Simulator::Remove(it->second);
  
  m_activeRequests.clear();
  m_timeoutUnsolicitedData.clear();
}

void
PDRMUnsolicited::SendUnsolicitedData(Name chunk)
{
  NS_LOG_INFO(chunk);
  // Create data packet
  auto data = make_shared<Data>();
  data->setName(chunk);
  data->setFreshnessPeriod(::ndn::time::milliseconds(m_freshness.GetMilliSeconds()));
  data->setContent(make_shared< ::ndn::Buffer>(m_virtualPayloadSize));
  data->setUnsolicited(true); 
 
  // Add signature
  Signature signature;
  SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));
  
  if (m_keyLocator.size() > 0) {
    signatureInfo.setKeyLocator(m_keyLocator);
  }
  
  signature.setInfo(signatureInfo);
  signature.setValue(::ndn::nonNegativeIntegerBlock(::ndn::tlv::SignatureValue, m_signature));
  
  data->setSignature(signature);
  
  // logging and stats
  m_pushedUnsolicitedData(this, data->getName());
  
  // Send it
  data->wireEncode();
  
  m_transmittedDatas(data, this, m_face);
  m_face->onReceiveData(*data);
}

// MOBILITY ACTIONS

void
PDRMUnsolicited::UnsolicitedMove(Ptr<const MobilityModel> model)
{
  NS_LOG_FUNCTION_NOARGS();
  PushUnsolicitedData();

  Simulator::Schedule(Time("50ms"), &PDRMUnsolicited::Move, this, model);
}

// This is equal to PDRMMobileProducer, but the Move part is different
void
PDRMUnsolicited::CourseChange(Ptr<const MobilityModel> model)
{
  if (!m_mobile)
    return;

  NS_LOG_FUNCTION_NOARGS();
  if (m_moving)
    Session(model);
  else
    UnsolicitedMove(model);

  NS_LOG_INFO(m_position.x);
}

} // namespace ndn
} // namespace ns3


