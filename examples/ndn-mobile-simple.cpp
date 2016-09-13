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
#include <ns3/ndnSIM/utils/ndn-mobility-profile.hpp>
#include <ns3/ndnSIM/utils/ndn-profile-container.hpp>

#include <iostream>
#include <fstream>

using namespace std;

namespace ns3 {

void
CacheEntryRemoved(std::string context, Ptr<const ndn::cs::Entry> entry, Time lifetime)
{
    cout << "Cache entry removed: " << context << " " << entry->GetName() << " " << lifetime.ToDouble(Time::S) << "s" << std::endl;
}

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
  string inputfile;
  string parameter;
  Time simulation_time;
  uint32_t seed;
  uint32_t run;
  string s_movement;
  Ptr<RandomVariableStream> movement;
  string s_session;
  Ptr<RandomVariableStream> session;
  uint32_t vicinity_size;
  uint32_t replication_degree;
  uint32_t n_nodes;
  uint32_t n_routers;
  uint32_t n_users;
  string topology;
  string data_rate;
  string delay;
  uint32_t max_packets;
  uint32_t catalog_size;
  uint32_t max_consumers;
  double popularity_alpha;
  double size_mean;
  double size_stddev;
//  double lifetime_mean;
//  double lifetime_stddev;
  uint32_t cache_size;
  uint32_t window_size;
  uint32_t payload_size;
  Time min_publish_time;
  Time max_publish_time;

  // Read command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.AddValue("input", "InputFile", inputfile);
  cmd.Parse(argc, argv);

  ifstream infile(inputfile + ".in");
  cout << "Begin simulation " << endl; 
  infile >> parameter >> simulation_time;
  cout << parameter << " " << simulation_time << endl;
  infile >> parameter >> seed;
  cout << parameter << " " << seed << endl;
  infile >> parameter >> run;
  cout << parameter << " " << run << endl;

  infile >> parameter >> s_movement;
  cout << parameter << " " << s_movement << endl;
  if (!s_movement.compare("Uniform"))
  {
    Time movement_min;
    Time movement_max;
    infile >> parameter >> movement_min;
    infile >> parameter >> movement_max;
    movement = CreateObject<UniformRandomVariable>();
    movement->SetAttribute("Min", DoubleValue(movement_min.GetMinutes()));
    movement->SetAttribute("Max", DoubleValue(movement_max.GetMinutes()));
  }
  else if (!s_movement.compare("Constant"))
  {
    Time movement_constant;
    infile >> parameter >> movement_constant;
    cout << parameter << " " << movement_constant << endl;
    movement = CreateObject<ConstantRandomVariable>();
    movement->SetAttribute("Constant", DoubleValue(movement_constant.GetMinutes()));
  }

  infile >> parameter >> s_session;
  cout << parameter << " " << s_session << endl;
  if (!s_session.compare("Pareto"))
  {
    double session_mean;
    double session_shape;
    double session_bound;
    infile >> parameter >> session_mean;
    infile >> parameter >> session_shape;
    infile >> parameter >> session_bound;
    session = CreateObject<ParetoRandomVariable>();
    session->SetAttribute("Mean", DoubleValue(session_mean));
    session->SetAttribute("Shape", DoubleValue(session_shape));
    session->SetAttribute("Bound", DoubleValue(session_bound));
  }
  else if (!s_session.compare("Constant"))
  {
    Time session_constant;
    infile >> parameter >> session_constant;
    cout << parameter << " " << session_constant << endl;
    session = CreateObject<ConstantRandomVariable>();
    session->SetAttribute("Constant", DoubleValue(session_constant.GetMinutes()));
  }

  ns3::ndn::MobilityProfileContainer userProfiles = ns3::ndn::MobilityProfileContainer();
  ns3::ndn::MobilityProfile mobility_profile = ns3::ndn::MobilityProfile();
  mobility_profile.SetNumberAp(3);
//  MobilityProfileContainer

  infile >> parameter >> vicinity_size;
  cout << parameter << " " << vicinity_size << endl;
  infile >> parameter >> replication_degree;
  cout << parameter << " " << replication_degree << endl;
  infile >> parameter >> n_nodes;
  cout << parameter << " " << n_nodes << endl;
  infile >> parameter >> n_routers;
  cout << parameter << " " << n_routers << endl;
  infile >> parameter >> n_users;
  cout << parameter << " " << n_users << endl;
  infile >> parameter >> topology;
  cout << parameter << " " << topology << endl;
//  infile >> parameter >> data_rate;
//  infile >> parameter >> delay;
//  infile >> parameter >> max_packets;
  infile >> parameter >> catalog_size;
  cout << parameter << " " << catalog_size << endl;
  infile >> parameter >> max_consumers;
  cout << parameter << " " << max_consumers << endl;
  infile >> parameter >> popularity_alpha;
  cout << parameter << " " << popularity_alpha << endl;
  infile >> parameter >> size_mean;
  cout << parameter << " " << size_mean << endl;
  infile >> parameter >> size_stddev;
  cout << parameter << " " << size_stddev << endl;
//  infile >> parameter >> lifetime_mean;
//  infile >> parameter >> lifetime_stddev;
  infile >> parameter >> cache_size;
  cout << parameter << " " << cache_size << endl;
  infile >> parameter >> window_size;
  cout << parameter << " " << window_size << endl;
  infile >> parameter >> payload_size;
  cout << parameter << " " << payload_size << endl;
  infile >> parameter >> min_publish_time;
  cout << parameter << " " << min_publish_time << endl;
  infile >> parameter >> max_publish_time;
  cout << parameter << " " << max_publish_time << endl;

  RngSeedManager::SetSeed(seed);
  RngSeedManager::SetRun(run);  

  AnnotatedTopologyReader topologyReader("", 1);
  topologyReader.SetFileName("/home/matheus/icn-2016/" + topology);
  topologyReader.Read();

  cout << "Creating nodes" << endl;
  NodeContainer routerNodes;
  for (uint32_t i = 0; i < n_routers; i++)
  {
    routerNodes.Add(Names::Find<Node>("Node" + to_string(i)));
  }

  NodeContainer userNodes;
  userNodes.Create(n_users);

  PointToPointHelper p2p;
  for (uint32_t i = 0; i < n_routers; i++)
  {
    for (uint32_t j = 0; j < n_users; j++)
    {
      p2p.Install(routerNodes.Get(i), userNodes.Get(j));
    }
  }

  cout << "Install NDN stack on all nodes" << endl;
  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetOldContentStore("ns3::ndn::cs::Stats::Lru", "MaxSize", to_string(cache_size));
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  //Config::Connect("/NodeList/*/$ns3::ndn::cs::Stats::Lru/WillRemoveEntry", MakeCallback(CacheEntryRemoved));

  cout << "Choosing forwarding strategy" << endl;
  // Choosing forwarding strategy
  ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::InstallAll("/hint", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::InstallAll("/vicinity", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::InstallAll("/prod0", "/localhost/nfd/strategy/best-route");
//  ndn::StrategyChoiceHelper::InstallAll("/prod0", "/localhost/nfd/strategy/multicast");

  cout << "Installing applications" << endl;
  // Installing applications
  Ptr<ns3::ndn::Catalog> catalog = Create<ns3::ndn::Catalog>();
  for (uint32_t i = 0; i < n_routers; i++)
  {
    catalog->addRouter(routerNodes.Get(i));
  }
  for (uint32_t i = 0; i < n_users; i++)
  {
    catalog->addUser(userNodes.Get(i));
  }

  catalog->setMaxSimulationTime(simulation_time);
  catalog->setUserPopulationSize(n_users);
  catalog->setPopularity(popularity_alpha, max_consumers);
  catalog->setObjectPopularityVariation(0);
  catalog->setObjectSize(size_mean, size_stddev);
//  catalog->setObjectLifetime(lifetime_mean, lifetime_stddev);
  catalog->initializeCatalog();

  cout << "Initialize positions of nodes" << endl;
  // Initialize positions of nodes
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  for (uint32_t i = 0; i < n_routers; i++) {
    positionAlloc->Add(Vector(i, 0, 0));
  }

  Ptr<UniformRandomVariable> initialization = CreateObject<UniformRandomVariable>();

  cout << "User" << endl;
  // User
  uint32_t producers = 0;
  for (uint32_t i = 0; i < n_users; i++) {
    Time publishTime = Minutes(initialization->GetValue(min_publish_time.GetMinutes(), max_publish_time.GetMinutes()));
    uint32_t pos;
    if (i == 0) pos = 2;
    else if (i == 1) pos = 0;
    else pos = (uint32_t) initialization->GetValue(0, n_routers);

    ndn::AppHelper consumerHelper("ns3::ndn::MobileUser");
    consumerHelper.SetPrefix("/prod" + to_string(i));
    consumerHelper.SetAttribute("Postfix", StringValue("/obj"));
    consumerHelper.SetAttribute("InitialWindowSize", UintegerValue(window_size));
    consumerHelper.SetAttribute("PayloadSize", UintegerValue(payload_size));
    if (producers < catalog_size)
    {
      consumerHelper.SetAttribute("PublishTime", StringValue(to_string(publishTime.ToDouble(Time::MIN)) + "min"));
      producers++;
    }
    consumerHelper.SetAttribute("VicinitySize", UintegerValue(vicinity_size));
    consumerHelper.SetAttribute("ReplicationDegree", UintegerValue(replication_degree));
    consumerHelper.SetAttribute("CacheSize", UintegerValue(cache_size));
    consumerHelper.SetAttribute("Catalog", PointerValue(catalog));
    consumerHelper.Install(userNodes.Get(i));

    for (uint32_t j = 0; j < n_routers; j++) {
      ndn::LinkControlHelper::FailLink(routerNodes.Get(j), userNodes.Get(i));
    }
    cout << "UPLINK: " << pos << " " << i << endl;
    ndn::LinkControlHelper::UpLink(routerNodes.Get(pos), userNodes.Get(i));
  }

  MobilityHelper mobility;

  cout << "Initialize positions of nodes" << endl;
  // Initialize positions of nodes
  mobility.SetPositionAllocator(positionAlloc);

  Ptr<ListPositionAllocator> positionAllocCons = CreateObject<ListPositionAllocator>();
  positionAllocCons->Add(Vector(0, 0, 0));
  mobility.SetPositionAllocator(positionAllocCons);

  mobility.SetMobilityModel("ns3::OnOffMobilityModel",
                            "Movement", PointerValue(movement),
                            "Session", PointerValue(session),
                            "PositionAllocator", PointerValue(positionAllocCons));

  mobility.Install(userNodes.Get(1));

  Ptr<ListPositionAllocator> positionAllocProd = CreateObject<ListPositionAllocator>();
  positionAllocProd->Add(Vector(2, 0, 0));
  positionAllocProd->Add(Vector(3, 0, 0));
  mobility.SetPositionAllocator(positionAllocProd);

  mobility.SetMobilityModel("ns3::OnOffMobilityModel",
                            "Movement", PointerValue(movement),
                            "Session", PointerValue(session),
                            "PositionAllocator", PointerValue(positionAllocProd));

  mobility.Install(userNodes.Get(0));
/*

  for (uint32_t i = 0; i < n_users; i++) {
    mobility.Install(userNodes.Get(i));
  }
  */
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.InstallAll();
  ndnGlobalRoutingHelper.AddOrigins("/prod0", userNodes.Get(0));

  // Calculate and install FIBs
  ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::PrintFIBs();

  Simulator::Stop(simulation_time);

  ndn::L3RateTracer::InstallAll(inputfile + "_rate-trace.txt", Seconds(1));
  ndn::CsTracer::InstallAll(inputfile + "_cs-trace.txt", Seconds(1));
  ndn::AppDelayTracer::InstallAll(inputfile + "_app-delays-trace.txt");

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
