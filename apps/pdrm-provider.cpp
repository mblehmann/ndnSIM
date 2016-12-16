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

#include "pdrm-provider.hpp"

#include "helper/ndn-fib-helper.hpp"
#include "helper/ndn-global-routing-helper.hpp"

using namespace std;

NS_LOG_COMPONENT_DEFINE("ndn.PDRMProvider");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(PDRMProvider);

TypeId
PDRMProvider::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::PDRMProvider")
      .SetGroupName("Ndn")
      .SetParent<PDRMConsumer>()
      .AddConstructor<PDRMProvider>()

      // Provider
      .AddAttribute("StorageSize",
                    "Number of objects that the provider can store and provide on behalf of others",
                    UintegerValue(3),
                    MakeUintegerAccessor(&PDRMProvider::m_storageSize),
                    MakeUintegerChecker<uint32_t>())

      // Content
      .AddAttribute("PayloadSize",
                    "Virtual payload size for Content packets",
                    UintegerValue(1024),
                    MakeUintegerAccessor(&PDRMProvider::m_virtualPayloadSize),
                    MakeUintegerChecker<uint32_t>())

      .AddAttribute("Freshness",
                    "Freshness of data packets, if 0, then unlimited freshness",
                    TimeValue(Seconds(0)),
                    MakeTimeAccessor(&PDRMProvider::m_freshness),
                    MakeTimeChecker())

      // Tracing
      .AddTraceSource("ServedData",
                      "Data served by the provider",
                      MakeTraceSourceAccessor(&PDRMProvider::m_servedData),
                      "ns3::ndn::PDRMProvider::ServedDataCallback")

      .AddTraceSource("AnnouncedPrefix",
                      "Announcement and Unannoncement of prefixes by the provider",
                      MakeTraceSourceAccessor(&PDRMProvider::m_announcedPrefix),
                      "ns3::ndn::PDRMProvider::AnnouncedPrefixCallback");

  return tid;
}

PDRMProvider::PDRMProvider()
{
}

/**
 *
 */
void
PDRMProvider::StartApplication() 
{
  PDRMConsumer::StartApplication();

  m_storage = vector<Name>(m_storageSize);
  m_storedObjects = 0;
  m_circularIndex = 0;

  Simulator::Schedule(m_global->getMaxSimulationTime() - Time("1ms"), &PDRMProvider::EndGame, this);
}

/**
 *
 */
void
PDRMProvider::StopApplication()
{
  App::StopApplication();
}

void
PDRMProvider::EndGame()
{
}

void
PDRMProvider::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest);

  if (!m_active)
    return;

  Name dataName(interest->getName());

  Name producerPrefix = dataName.getPrefix(1);
  Name objectPrefix = dataName.getPrefix(2);

  if (m_announcedPrefixes.count(producerPrefix) == 0 && m_announcedPrefixes.count(objectPrefix) == 0)
    return;

//  NS_LOG_INFO(interest->getName());
  
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

/**
 *
 */
void
PDRMProvider::AnnouncePrefix(Name prefix)
{
  NS_LOG_INFO(prefix);
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper = m_global->getGlobalRoutingHelper();

  m_announcedPrefixes.insert(prefix);
  m_announcedPrefix(this, prefix, true);
  ndnGlobalRoutingHelper.AddOrigin(prefix.toUri(), this->GetNode());

  FibHelper::AddRoute(GetNode(), prefix, m_face, 0);

  if (m_updateNetwork.IsRunning()) {
    Simulator::Remove(m_updateNetwork);
  }
  m_updateNetwork = Simulator::Schedule(Time("50ms"), &PDRMProvider::UpdateNetwork, this);
}

void
PDRMProvider::UnannouncePrefix(Name prefix)
{
  NS_LOG_INFO(prefix);
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper = m_global->getGlobalRoutingHelper();

  m_announcedPrefixes.erase(prefix);
  m_announcedPrefix(this, prefix, false);
  ndnGlobalRoutingHelper.RemoveOrigin(prefix.toUri(), this->GetNode());

  FibHelper::RemoveRoute(GetNode(), prefix, m_face);

  if (m_updateNetwork.IsRunning()) {
    Simulator::Remove(m_updateNetwork);
  }
  m_updateNetwork = Simulator::Schedule(Time("50ms"), &PDRMProvider::UpdateNetwork, this);
}

void
PDRMProvider::UpdateNetwork()
{
  NS_LOG_FUNCTION_NOARGS();

  ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::PrintFIBs();
}

void
PDRMProvider::StoreObject(Name object)
{
  NS_LOG_FUNCTION_NOARGS();
  if (m_storedObjects == m_storageSize)
    DeleteObject();

  m_storage[m_circularIndex] = object;

  m_circularIndex = (m_circularIndex + 1) % m_storageSize;
  m_storedObjects = min(m_storedObjects+1, m_storageSize);
}

void
PDRMProvider::DeleteObject()
{
  NS_LOG_FUNCTION_NOARGS();
  Name object = m_storage[m_circularIndex];
  UnannouncePrefix(object);
}

} // namespace ndn
} // namespace ns3


