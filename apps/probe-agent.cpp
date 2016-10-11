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

#include "utils/ndn-ns3-packet-tag.hpp"

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
      .AddConstructor<ProbeAgent>()

      .AddAttribute("Prefix", "Prefix, for which producer has the data",
                    StringValue("/ha0"), MakeNameAccessor(&ProbeAgent::m_prefix),
                    MakeNameChecker());

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

  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);
}

void
ProbeAgent::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}

/**
 * Receives an interest.
 * If it is an update request, update the locator for the prefix (/update/prefix/locator)
 * If it is a register request, register the locator for the prefix (/register/prefix/locator)
 * If it is an unregister request, unregister the locator for the prefix (/unregister/prefix)
 * If it is an object request, pre-append the locator and forward it
 */
void
ProbeAgent::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // tracing inside

  NS_LOG_FUNCTION_NOARGS();
  NS_LOG_DEBUG("Node" << GetNode()->GetId() << " receiving interest " << interest->getName().toUri());

  if (!m_active)
    return;

  if (interest->getName().at(1).toUri() == "update") {
    Update("/" + interest->getName().at(2).toUri(), "/" + interest->getName().at(3).toUri());
  } else if (interest->getName().at(1).toUri() == "register") {
    Register("/" + interest->getName().at(2).toUri(), "/" + interest->getName().at(3).toUri());
  } else if (interest->getName().at(1).toUri() == "unregister") {
    Unregister("/" + interest->getName().at(2).toUri());
  } else {
    string prefix = "/" + interest->getName().at(0).toUri();
    shared_ptr<Name> forwardName = make_shared<Name>(m_prefixes[prefix]);
    forwardName->append(interest->getName());

    shared_ptr<Interest> forwardInterest = make_shared<Interest>();
    forwardInterest->setNonce(interest->getNonce());
    forwardInterest->setName(*forwardName);

    m_transmittedInterests(forwardInterest, this, m_face);
    m_face->onReceiveInterest(*forwardInterest);
    NS_LOG_DEBUG("Node" << GetNode()->GetId() << " forwarding interest " << forwardName->toUri());
  }
}

void
ProbeAgent::OnData(shared_ptr<const Data> data)
{
  if (!m_active)
    return;

  App::OnData(data); // tracing inside

  NS_LOG_FUNCTION_NOARGS();
  NS_LOG_DEBUG("Node" << GetNode()->GetId() << " receiving data " << data->getName().toUri());

  int hopCount = 0;
  auto ns3PacketTag = data->getTag<Ns3PacketTag>();
  if (ns3PacketTag != nullptr) { // e.g., packet came from local node's cache
    FwHopCountTag hopCountTag;
    if (ns3PacketTag->getPacket()->PeekPacketTag(hopCountTag)) {
      hopCount = hopCountTag.Get();
      NS_LOG_DEBUG("Hop count: " << hopCount);
    }
  }

  string prefix = data->getName().at(0).toUri();

  Name objectName(data->getName().getSubName(1));
  objectName.append(to_string(hopCount));
  NS_LOG_DEBUG(objectName.toUri());
  
  auto forwardData = make_shared<Data>();
  forwardData->setName(objectName);
  forwardData->setFreshnessPeriod(data->getFreshnessPeriod());

  forwardData->setContent(data->getContent());
  forwardData->setSignature(data->getSignature());

  // to create real wire encoding
  forwardData->wireEncode();

  m_transmittedDatas(forwardData, this, m_face);
  m_face->onReceiveData(*forwardData);
  NS_LOG_DEBUG("Node" << GetNode()->GetId() << " forwarding interest " << objectName.toUri());
}

void
ProbeAgent::Register(string prefix, string locator)
{
  NS_LOG_FUNCTION_NOARGS();
  m_prefixes[prefix] = locator;
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper; 
  ndnGlobalRoutingHelper.AddOrigin(prefix, GetNode());
  FibHelper::AddRoute(GetNode(), prefix, m_face, 0);
  ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::PrintFIBs();
  NS_LOG_INFO("Node" << GetNode()->GetId() << " registering " << prefix << " => " << locator);
}

void
ProbeAgent::Update(string prefix, string locator)
{
  NS_LOG_FUNCTION_NOARGS();
  m_prefixes[prefix] = locator;
  NS_LOG_INFO("Node" << GetNode()->GetId() << " updating " << prefix << " => " << locator);
}

void
ProbeAgent::Unregister(string prefix)
{
  NS_LOG_FUNCTION_NOARGS();
  m_prefixes.erase(prefix);
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
//  ndnGlobalRoutingHelper.RemoveOrigin(prefix, GetNode());
  FibHelper::RemoveRoute(GetNode(), prefix, m_face);
  ndn::GlobalRoutingHelper::CalculateRoutes();
  NS_LOG_DEBUG("Node" << GetNode()->GetId() << " unregistering " << prefix);
}

} // namespace ndn
} // namespace ns3
