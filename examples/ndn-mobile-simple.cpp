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

  double mean = 100;
  double shape = 1.07;

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.AddValue("mean", "Mean", mean);
  cmd.AddValue("shape", "Shape", shape);;
  cmd.Parse(argc, argv);

  //paramaters
  //simulationTime
  Time simulationTime = Seconds(2.0);

  //seed
  RngSeedManager::SetSeed(3);
  RngSeedManager::SetRun(7);

  // TODO write a topology file
  //numberUsers
  uint32_t numberUsers = 2;
  uint32_t numberRouters = 3;

  //mobilityParameters;
  Ptr<UniformRandomVariable> speed = CreateObject<UniformRandomVariable>();
  speed->SetAttribute("Min", DoubleValue(0.3));
  speed->SetAttribute("Max", DoubleValue(3.0));

  //StringValue speed = StringValue("ns3::UniformRandomVariable[Min=0.3|Max=3.0]");

  //pauseParameters;
  Ptr<ParetoRandomVariable> pause = CreateObject<ParetoRandomVariable>();
  pause->SetAttribute("Mean", DoubleValue(mean));
  pause->SetAttribute("Shape", DoubleValue(shape));
  pause->SetAttribute("Bound", DoubleValue(180));

  //vicinitySize
  uint32_t vicinitySize = 2;

  //replicationDegree
  uint32_t replicationDegree = 3;

  //numberObjects
  uint32_t catalogSize = 1;  

  //cacheSize;
  uint32_t cacheSize = 1;

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
  ns->addRouter(nodes.Get(1));
  ns->addRouter(nodes.Get(2));
  ns->addRouter(nodes.Get(3));
  ns->setCatalogSize(10);
  ns->initializeObjectSize(100, 20);
  ns->initializePopularity(0.8);

  // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::MobileUser");
  consumerHelper.SetPrefix("/prod0");
  consumerHelper.SetAttribute("Postfix", StringValue("/obj"));
  consumerHelper.SetAttribute("WindowSize", StringValue("3"));
  consumerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  consumerHelper.SetAttribute("NameService", PointerValue(ns));
  consumerHelper.Install(nodes.Get(0));                        // first node

  // Producer
  //ndn::AppHelper producerHelper("ns3::ndn::Producer");
  ndn::AppHelper producerHelper("ns3::ndn::MobileUser");
  producerHelper.SetPrefix("/prod4");
  producerHelper.SetAttribute("Postfix", StringValue("/obj"));
  producerHelper.SetAttribute("WindowSize", StringValue("3"));
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.SetAttribute("NameService", PointerValue(ns));
  producerHelper.Install(nodes.Get(4)); // last node

  // Define starting position of node 4
  ndn::LinkControlHelper::FailLink(nodes.Get(2), nodes.Get(4));
  ndn::LinkControlHelper::UpLink(nodes.Get(3), nodes.Get(4));

  MobilityHelper mobility;

  // Initialize positions of nodes
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  positionAlloc->Add(Vector(0, 0, 0));
  positionAlloc->Add(Vector(10, 0, 0));
  positionAlloc->Add(Vector(0, 10, 0));
  positionAlloc->Add(Vector(10, 10, 0));

  mobility.SetPositionAllocator(positionAlloc);

  mobility.SetMobilityModel("ns3::RandomWaypointMobilityModel",
                            "Speed", PointerValue(speed),
                            "Pause", PointerValue(pause),
                            "PositionAllocator", PointerValue(positionAlloc));

  mobility.Install(nodes.Get(0));
  mobility.Install(nodes.Get(4));

  Simulator::Stop(simulationTime);

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
