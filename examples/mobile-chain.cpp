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

// ndn-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

#include "ns3/mobility-module.h"
#include <ns3/ndnSIM/utils/ndn-catalog.hpp>
#include "model/ndn-l3-protocol.hpp"
#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"

namespace ns3 {

/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +----------+     1Mbps      +--------+     1Mbps      +----------+
 *      | consumer | <------------> | router | <------------> | producer |
 *      +----------+         10ms   +--------+          10ms  +----------+
 *
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-simple
 */

int
main(int argc, char* argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("20"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  uint32_t n = 4;

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(n);
  Ptr<Node> producer = nodes.Get(3);
  Ptr<Node> homeagent = nodes.Get(1);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  for (uint32_t i = 0; i < n-2; i++) {
   p2p.Install(nodes.Get(i), nodes.Get(i+1));
  }

  for (uint32_t i = 0; i < n-1; i++) {
   p2p.Install(nodes.Get(i), producer);
  }

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/best-route");
  ndn::StrategyChoiceHelper::InstallAll("/hint", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::InstallAll("/vicinity", "/localhost/nfd/strategy/multicast");

  //ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  //ndnGlobalRoutingHelper.InstallAll();

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  Ptr<ns3::ndn::Catalog> catalog = Create<ns3::ndn::Catalog>();
  for (uint32_t i = 0; i < n-1; i++)
  {
    catalog->addRouter(nodes.Get(i));
    positionAlloc->Add(Vector(i, 0, 0));
  }

  // Installing applications

  string globalprefix = "/global";
  string localprefix = "/local";

  // Probe Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::ProbeConsumer");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix(globalprefix);
  consumerHelper.SetAttribute("Frequency", StringValue("1")); // 10 interests a second
  consumerHelper.Install(nodes.Get(0));                        // first node

  ndn::AppHelper agentHelper("ns3::ndn::ProbeAgent");
  agentHelper.Install(homeagent);

  // Producer
  
  ndn::AppHelper producerHelper("ns3::ndn::ProbeProducer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix(localprefix);
  producerHelper.SetAttribute("PayloadSize", StringValue("0"));
  producerHelper.SetAttribute("Catalog", PointerValue(catalog));
  producerHelper.Install(producer); // last node

  for (uint32_t i = 0; i < n-1; i++) {
    ndn::LinkControlHelper::FailLink(nodes.Get(i), producer);
  }
  ndn::LinkControlHelper::UpLink(nodes.Get(1), producer);

  // Mobility
  MobilityHelper mobility;

  Ptr<RandomVariableStream> movement;
  movement = CreateObject<ConstantRandomVariable>();
  movement->SetAttribute("Constant", DoubleValue(1));
  
  Ptr<RandomVariableStream> session;
  session = CreateObject<ConstantRandomVariable>();
  session->SetAttribute("Constant", DoubleValue(0.5));

  // Initialize positions of nodes
  mobility.SetPositionAllocator(positionAlloc);

  mobility.SetMobilityModel("ns3::OnOffMobilityModel",
                            "Movement", PointerValue(movement),
                            "Session", PointerValue(session),
                            "PositionAllocator", PointerValue(positionAlloc));

  mobility.Install(producer);

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();
  ndnGlobalRoutingHelper.AddOrigins(globalprefix, homeagent);
  ndnGlobalRoutingHelper.AddOrigins(localprefix, producer);

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::PrintFIBs();  

  Simulator::Stop(Seconds(200.0));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
