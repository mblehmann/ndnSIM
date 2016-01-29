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

// ndn-mobile-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"

#include "ns3/mobility-module.h"

#include <ns3/ndnSIM/utils/ndn-catalog.hpp>

namespace ns3 {

/**
 * This scenario simulates a very simple network topology:
 *
 *                                    1Mbps   +--------+  1Mbps
 *                                   <------> | router | <------>
 *      +------+  1Mbps   +--------+   10ms   +--------+   10ms  +----------+
 *      | user | <------> | router |                             | producer |
 *      +------+   10ms   +--------+  1Mbps   +--------+  1Mbps  +----------+
 *                                   <------> | router | <------>
 *                                     10ms   +--------+   10ms
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.MobileUser ./waf --run=ndn-mobile-simple
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

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(5);

  // Connecting nodes using 4 links
  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(1));
  p2p.Install(nodes.Get(1), nodes.Get(2));
  p2p.Install(nodes.Get(1), nodes.Get(3));
  p2p.Install(nodes.Get(2), nodes.Get(4));
  p2p.Install(nodes.Get(3), nodes.Get(4));

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  //ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/multicast");

  // Installing applications

  Ptr<ns3::ndn::NameService> ns = Create<ns3::ndn::NameService>();
  ns->addUser(nodes.Get(0));
  ns->addUser(nodes.Get(4));

  // Consumer
  //ndn::AppHelper consumerHelper("ns3::ndn::ConsumerCbr");
  ndn::AppHelper consumerHelper("ns3::ndn::MobileUser");
  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix("/prod0");
  consumerHelper.SetAttribute("Postfix", StringValue("/obj"));
  consumerHelper.SetAttribute("WindowSize", StringValue("3"));
  consumerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  consumerHelper.SetAttribute("NameService", PointerValue(ns));
  consumerHelper.Install(nodes.Get(0));                        // first node

  // Producer
  //ndn::AppHelper producerHelper("ns3::ndn::Producer");
  ndn::AppHelper producerHelper("ns3::ndn::MobileUser");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix("/prod4");
  producerHelper.SetAttribute("Postfix", StringValue("/obj"));
  producerHelper.SetAttribute("WindowSize", StringValue("3"));
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.SetAttribute("NameService", PointerValue(ns));
  producerHelper.Install(nodes.Get(4)); // last node

  // Define starting position of node 4
  ndn::LinkControlHelper::FailLink(nodes.Get(2), nodes.Get(4));
  ndn::LinkControlHelper::UpLink(nodes.Get(3), nodes.Get(4));

  Ptr<UniformRandomVariable> randomizer = CreateObject<UniformRandomVariable>();
  randomizer->SetAttribute("Min", DoubleValue(10));
  randomizer->SetAttribute("Max", DoubleValue(100));

  MobilityHelper mobility;

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  positionAlloc->Add(Vector(0, 0, 0));
  positionAlloc->Add(Vector(10, 0, 0));
  positionAlloc->Add(Vector(0, 10, 0));
  positionAlloc->Add(Vector(10, 10, 0));

  mobility.SetPositionAllocator(positionAlloc);

  mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                            "Speed", StringValue("ns3::UniformRandomVariable[Min=0.3|Max=0.7]"),
                            "Pause", StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),
                            "PositionAllocator", PointerValue(positionAlloc));



/*
  // Mobility
  MobilityHelper mobility;
  // Put everybody into a line
  Ptr<ListPositionAllocator> initialAlloc = CreateObject<ListPositionAllocator>();
//  for (uint32_t i = 0; i < mainNodes.GetN(); ++i) {
    initialAlloc->Add(Vector(0, 0, 0));
    initialAlloc->Add(Vector(1, 1, 0));
    initialAlloc->Add(Vector(2, 2, 0));
    initialAlloc->Add(Vector(3, 3, 0));
    initialAlloc->Add(Vector(4, 4, 0));
//  }
  mobility.SetPositionAllocator(initialAlloc);

  ObjectFactory pos;
  pos.SetTypeId ("ns3::GridPositionAllocator");
  pos.Set ("MinX", DoubleValue (0.0));
  pos.Set ("MinY", DoubleValue (0.0));
  pos.Set ("DeltaX", DoubleValue (5.0));
  pos.Set ("DeltaY", DoubleValue (5.0));
  pos.Set ("GridWidth", UintegerValue (10));
  pos.Set ("LayoutType", StringValue ("RowFirst"));

  Ptr<Object> posAlloc =(pos.Create());

  mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                            "Speed", StringValue("ns3::UniformRandomVariable[Min=0.3|Max=0.7]"),
                            "Pause", StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),
                            "PositionAllocator", PointerValue(posAlloc));
*/
  mobility.Install(nodes.Get(0));
  mobility.Install(nodes.Get(4));

  Simulator::Stop(Seconds(100.0));

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
