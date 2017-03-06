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

#include <ns3/ndnSIM/utils/pdrm-catalog.hpp>
#include <ns3/ndnSIM/utils/pdrm-global.hpp>

#include "ns3/mobility-module.h"

#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"

namespace ns3 {

/**
 * This scenario simulates a binary tree network topology with height 4:
 *
 * 
 *      +----------+     1Gbps      +--------+     1Gbps      +----------+
 *      | consumer | <------------> | router | <------------> | producer |
 *      +----------+         30ms   +--------+          30ms  +----------+
 *
 *
 * Consumer requests data from producer with a data rate of 2Mbps
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.PDRMConsumer:ndn.PDRMProducer ./waf --run=pdrm-simple
 */

struct Configuration {
 uint32_t seed;
 uint32_t run;
 uint32_t scenario;
 double time;

 string topology;
 uint32_t routers;
 uint32_t mobile_producers;
 uint32_t producer_availability;
 string data_rate;
 string delay;
 string max_packets;
 double cache_size;

 string producer_prefix;
 string object_prefix;
 uint32_t object_size;
 uint32_t catalog_size;
 double catalog_alpha;

 string lambda;
 string chunk_size;
 string start;
 string end;
 string warmup_period;
 string lifetime;

 string storage_size;
 string freshness;
 string production_interval;

 string period;
 double availability;
 double variance;

 string hint_prefix;
 string hint_timer;

 string vicinity_prefix;
 string placement_policy;
 string vicinity_size;
 string vicinity_timer;
};

Configuration
parseInput(string inputfile)
{
 Configuration c;
 string parameter;

 ifstream infile(inputfile);
 infile >> parameter >> c.seed;
 infile >> parameter >> c.run;
 infile >> parameter >> c.scenario;
 infile >> parameter >> c.time;

 infile >> parameter >> c.topology;
 infile >> parameter >> c.routers;
 infile >> parameter >> c.mobile_producers;
 infile >> parameter >> c.producer_availability;
 infile >> parameter >> c.data_rate;
 infile >> parameter >> c.delay;
 infile >> parameter >> c.max_packets;
 infile >> parameter >> c.cache_size;

 infile >> parameter >> c.producer_prefix;
 infile >> parameter >> c.object_prefix;
 infile >> parameter >> c.object_size;
 infile >> parameter >> c.catalog_size;
 infile >> parameter >> c.catalog_alpha;

 infile >> parameter >> c.lambda;
 infile >> parameter >> c.chunk_size;
 infile >> parameter >> c.start;
 infile >> parameter >> c.end;
 infile >> parameter >> c.warmup_period;
 infile >> parameter >> c.lifetime;

 infile >> parameter >> c.storage_size;
 infile >> parameter >> c.freshness;
 infile >> parameter >> c.production_interval;

 infile >> parameter >> c.period;
 infile >> parameter >> c.availability;
 infile >> parameter >> c.variance;

 infile >> parameter >> c.vicinity_prefix;
 infile >> parameter >> c.placement_policy;
 infile >> parameter >> c.vicinity_size;
 infile >> parameter >> c.vicinity_timer;

 infile >> parameter >> c.hint_prefix;
 infile >> parameter >> c.hint_timer;

 return c;
}

int
main(int argc, char* argv[])
{
  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  string inputfile;
  cmd.AddValue("input", "InputFile", inputfile);
  cmd.Parse(argc, argv);

  Configuration c = parseInput(inputfile);

  // setting default parameters for PointToPoint links and channels
  RngSeedManager::SetSeed(c.seed);
  RngSeedManager::SetRun(c.run);

  string scenario_prefix = "";
  if (c.scenario == 0)
    scenario_prefix = "unavail-";

  Time simulationTime = Seconds(c.time);

  AnnotatedTopologyReader topologyReader("", 1);
  topologyReader.SetFileName(c.topology);
  topologyReader.Read();


  // Creating nodes
  NodeContainer nodes;
  for (uint32_t i = 0; i < c.routers; i++)
    nodes.Add(Names::Find<Node>("Node" + to_string(i)));
  
  nodes.Create(c.mobile_producers);

  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue(c.data_rate));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue(c.delay));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue(c.max_packets));

  PointToPointHelper p2p;
  for (uint32_t i = 0; i < c.mobile_producers; i++)
    p2p.Install(nodes.Get(0), nodes.Get(c.routers+i));

  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  if (c.cache_size)
    ndnHelper.SetOldContentStore("ns3::ndn::cs::Stats::Lru", "MaxSize", to_string((int) (c.object_size * c.catalog_size * c.cache_size)));
  else
    ndnHelper.SetOldContentStore("ns3::ndn::cs::Nocache");
  ndnHelper.InstallAll();

  ndn::StrategyChoiceHelper::InstallAll("/vicinity", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::InstallAll("/hint", "/localhost/nfd/strategy/multicast");
  for (uint32_t i = 0; i < c.mobile_producers; i++)
    ndn::StrategyChoiceHelper::InstallAll(c.producer_prefix + to_string(c.routers+i), "/localhost/nfd/strategy/best-route");

  // Choosing forwarding strategy
  ndn::GlobalRoutingHelper grHelper;
  grHelper.InstallAll();

  // Installing applications
  Ptr<ns3::ndn::PDRMCatalog> catalog = Create<ns3::ndn::PDRMCatalog>();
  catalog->setObjectSize(c.object_size);
  catalog->initializeCatalog(c.catalog_size, c.catalog_alpha);

  vector<Ptr<Node> > routers;
  for (uint32_t i = 0; i < c.routers; i++)
    routers.push_back(nodes.Get(i));

  Ptr<ns3::ndn::PDRMGlobal> global = Create<ns3::ndn::PDRMGlobal>();
  global->setRouters(routers);
  global->setMaxSimulationTime(simulationTime);
  global->setGlobalRoutingHelper(grHelper);

  // Consumer
  ndn::AppHelper consumer("ns3::ndn::PDRMConsumer");
  consumer.SetAttribute("Lambda", StringValue(c.lambda));
  consumer.SetAttribute("Start", StringValue(c.start));
  consumer.SetAttribute("End", StringValue(c.end));
  consumer.SetAttribute("WarmupPeriod", StringValue(c.warmup_period));
  consumer.SetAttribute("Lifetime", StringValue(c.lifetime));
  consumer.SetAttribute("DefaultConsumer", StringValue("true"));
  consumer.SetAttribute("Catalog", PointerValue(catalog)); 
  consumer.SetAttribute("Global", PointerValue(global));
  // install on leaves
  for (uint32_t i = c.routers / 2; i < c.routers; i++)
    consumer.Install(nodes.Get(i));

  // Mobile Producer
  ndn::AppHelper mobileProvider("ns3::ndn::PDRMStrategy");
  mobileProvider.SetAttribute("Start", StringValue(c.start));
  mobileProvider.SetAttribute("End", StringValue(c.end));
  mobileProvider.SetAttribute("WarmupPeriod", StringValue(c.warmup_period));
  mobileProvider.SetAttribute("DefaultConsumer", StringValue("false"));
  mobileProvider.SetAttribute("Catalog", PointerValue(catalog)); 
  mobileProvider.SetAttribute("Global", PointerValue(global));

  mobileProvider.SetAttribute("StorageSize", StringValue(c.storage_size));
  mobileProvider.SetAttribute("PayloadSize", StringValue(c.chunk_size));
  mobileProvider.SetAttribute("Freshness", StringValue(c.freshness)); 

  mobileProvider.SetAttribute("ProducingInterval", StringValue(c.production_interval)); 
  mobileProvider.SetAttribute("ProducerPrefix", StringValue(c.producer_prefix)); 
  mobileProvider.SetAttribute("ObjectPrefix", StringValue(c.object_prefix)); 
  if (c.availability == 1)
    mobileProvider.SetAttribute("Mobile", StringValue("false"));
  else
    mobileProvider.SetAttribute("Mobile", StringValue("true")); 

  mobileProvider.SetAttribute("VicinityPrefix", StringValue(c.vicinity_prefix)); 
  mobileProvider.SetAttribute("PlacementPolicy", StringValue(c.placement_policy)); 
  mobileProvider.SetAttribute("VicinitySize", StringValue(c.vicinity_size));  
  mobileProvider.SetAttribute("VicinityTimer", StringValue(c.vicinity_timer)); 

  mobileProvider.SetAttribute("HintPrefix", StringValue(c.hint_prefix)); 
  mobileProvider.SetAttribute("HintTimer", StringValue(c.hint_timer)); 

  for (uint32_t i = 0; i < c.mobile_producers; i++) {
    mobileProvider.Install(nodes.Get(c.routers+i));
    mobileProvider.SetAttribute("ProducingInterval", StringValue("0s")); 
  }

  // Mobile
  MobilityHelper mobility;

  // Initialize positions of nodes
  Time session_period;
  Ptr<RandomVariableStream> session;

  if (c.availability == 1) {
    session_period = simulationTime;
    session = CreateObject<ConstantRandomVariable>();
    session->SetAttribute("Constant", DoubleValue(session_period.GetSeconds()));
  }
  else {
    session_period = max(Time::FromDouble(Time(c.period).GetSeconds() * c.availability, Time::S), Time("1ns"));
    session = CreateObject<NormalRandomVariable>();
    session->SetAttribute("Mean", DoubleValue(session_period.GetSeconds()));
    session->SetAttribute("Variance", DoubleValue(session_period.GetSeconds()*c.variance));
    session->SetAttribute("Bound", DoubleValue(session_period.GetSeconds()*(c.variance*2)));
  }

  Time movement_period = max(Time(c.period) - session_period, Time("1ns"));
  Ptr<RandomVariableStream> movement = CreateObject<NormalRandomVariable>();
  movement->SetAttribute("Mean", DoubleValue(movement_period.GetSeconds()));
  movement->SetAttribute("Variance", DoubleValue(movement_period.GetSeconds()*c.variance));
  movement->SetAttribute("Bound", DoubleValue(movement_period.GetSeconds()*(c.variance*2)));

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
  positionAlloc->Add(Vector(0, 0, 0));

  mobility.SetPositionAllocator(positionAlloc);
  mobility.SetMobilityModel("ns3::OnOffMobilityModel",
                            "Session", PointerValue(session),
                            "Movement", PointerValue(movement),
                            "PositionAllocator", PointerValue(positionAlloc));

  mobility.Install(nodes.Get(c.routers));
  ndn::LinkControlHelper::FailLink(nodes.Get(0), nodes.Get(c.routers));

  for (uint32_t i = 1; i < c.mobile_producers; i++) {
    // Mobile
    MobilityHelper mobilityProvider;

    double providerAvailability = c.producer_availability / 10.0;

    // Initialize positions of nodes
    Time session_period_provider;
    Ptr<RandomVariableStream> session_provider;

    if (providerAvailability == 1) {
      session_period_provider = simulationTime;
      session_provider = CreateObject<ConstantRandomVariable>();
      session_provider->SetAttribute("Constant", DoubleValue(session_period_provider.GetSeconds()));
    }
    else {
      session_period_provider = max(Time::FromDouble(Time(c.period).GetSeconds() * providerAvailability, Time::S), Time("1ns"));
      session_provider = CreateObject<NormalRandomVariable>();
      session_provider->SetAttribute("Mean", DoubleValue(session_period_provider.GetSeconds()));
      session_provider->SetAttribute("Variance", DoubleValue(session_period_provider.GetSeconds()*c.variance));
      session_provider->SetAttribute("Bound", DoubleValue(session_period_provider.GetSeconds()*(c.variance*2)));
    }

    Time movement_period_provider = max(Time(c.period) - session_period_provider, Time("1ns"));
    Ptr<RandomVariableStream> movement_provider = CreateObject<NormalRandomVariable>();
    movement_provider->SetAttribute("Mean", DoubleValue(movement_period_provider.GetSeconds()));
    movement_provider->SetAttribute("Variance", DoubleValue(movement_period_provider.GetSeconds()*c.variance));
    movement_provider->SetAttribute("Bound", DoubleValue(movement_period_provider.GetSeconds()*(c.variance*2)));

    Ptr<ListPositionAllocator> positionAlloc_provider = CreateObject<ListPositionAllocator>();
    positionAlloc_provider->Add(Vector(0, 0, 0));

    mobilityProvider.SetPositionAllocator(positionAlloc_provider);
    mobilityProvider.SetMobilityModel("ns3::OnOffMobilityModel",
                              "Session", PointerValue(session_provider),
                              "Movement", PointerValue(movement_provider),
                              "PositionAllocator", PointerValue(positionAlloc_provider));

    mobilityProvider.Install(nodes.Get(c.routers+i));
    ndn::LinkControlHelper::FailLink(nodes.Get(0), nodes.Get(c.routers+i));
  }

  Simulator::Stop(simulationTime);

  ndn::PDRMConsumerTracer::InstallAll(scenario_prefix + inputfile + "-consumer.txt");
  ndn::PDRMProducerTracer::InstallAll(scenario_prefix + inputfile + "-producer.txt");
  ndn::PDRMMobileTracer::InstallAll(scenario_prefix + inputfile + "-mobility.txt");
  ndn::PDRMProposalTracer::InstallAll(scenario_prefix + inputfile + "-proposal.txt");

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
