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

#include "ns3/ndnSIM/utils/ndn-mobility-profile.hpp"
#include "ns3/ndnSIM/helper/ndn-profile-container.hpp"

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
  string s_session;
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
  string profiles;

  // Read command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.AddValue("input", "InputFile", inputfile);
  cmd.Parse(argc, argv);

  ifstream infile(inputfile + ".in");
  
  while(!infile.eof())
  {
    infile >> parameter;
    if(parameter == "simulation_time")
    {
      infile >> simulation_time;
      cout << simulation_time << endl;
    }
    else if(parameter == "seed")
      infile >> seed;
    else if(parameter == "run")
      infile >> run;
    else if(parameter == "vicinity_size")
      infile >> vicinity_size; 
    else if(parameter == "replication_degree")
      infile >> replication_degree;
    else if(parameter == "number_nodes")
      infile >> n_nodes;
    else if(parameter == "number_routers")
      infile >> n_routers;
    else if(parameter == "number_users")
      infile >> n_users;
    else if(parameter == "topology")
      infile >> topology;
    else if(parameter == "catalog_size")
      infile >> catalog_size;
    else if(parameter == "max_consumers")
      infile >> max_consumers;
    else if(parameter == "popularity_alpha")
      infile >> popularity_alpha;
    else if(parameter == "size_mean")
      infile >> size_mean;
    else if(parameter == "size_stddev")
      infile >> size_stddev;
    else if(parameter == "cache_size")
      infile >> cache_size;
    else if(parameter == "window_size")
      infile >> window_size;
    else if(parameter == "payload_size")
      infile >> payload_size;
    else if(parameter == "min_publish_time")
      infile >> min_publish_time;
    else if(parameter == "max_publish_time")
      infile >> max_publish_time;
    else if(parameter == "max_packets")
      infile >> max_packets;
    else if(parameter == "profiles_file")
      infile >> profiles;
  }

  ns3::ndn::MobilityProfileContainer userProfiles = ns3::ndn::MobilityProfileContainer();
  ifstream profile_input("/home/lleal/scripts-ndnSIM/" + profiles);
  
  while(!profile_input.eof()) {
    profile_input >> parameter;
    if(parameter == "##begin-profile") {
      shared_ptr<ns3::ndn::MobilityProfile> mobility_profile = make_shared<ns3::ndn::MobilityProfile>();
      while (true) {
      profile_input >> parameter;
      if (parameter == "number_AP") {
        uint32_t nAP;
        profile_input >> nAP;
        mobility_profile->SetNumberAp(nAP);
      }
      
      else if(parameter == "movement_distribution") {
        profile_input >> s_movement;
        if (!s_movement.compare("Uniform")) {
          Time movement_min;
          Time movement_max;
          profile_input >> parameter >> movement_min;
          profile_input >> parameter >> movement_max;
          Ptr<RandomVariableStream> Movement = mobility_profile->GetSession();
          Movement = CreateObject<UniformRandomVariable>();
          Movement->SetAttribute("Min", DoubleValue(movement_min.GetMinutes()));
          Movement->SetAttribute("Max", DoubleValue(movement_max.GetMinutes()));
        }
        
        else if (!s_movement.compare("Constant")) {
          Time movement_constant;
          profile_input >> parameter >> movement_constant;
          Ptr<RandomVariableStream> Movement = mobility_profile->GetSession();
          Movement = CreateObject<ConstantRandomVariable>();
          Movement->SetAttribute("Constant", DoubleValue(movement_constant.GetMinutes()));
        }
      }
      
      else if(parameter == "session_distribution") {
        profile_input >> s_session;
        if (!s_session.compare("Pareto")) {
          double session_mean;
          double session_shape;
          double session_bound;
          profile_input >> parameter >> session_mean;
          profile_input >> parameter >> session_shape;
          profile_input >> parameter >> session_bound;
          Ptr<RandomVariableStream> Session = mobility_profile->GetSession();
          Session = CreateObject<UniformRandomVariable>();
          Session->SetAttribute("Mean", DoubleValue(session_mean));
          Session->SetAttribute("Shape", DoubleValue(session_shape));
          Session->SetAttribute("Bound", DoubleValue(session_bound));
        }
        
        else if (!s_session.compare("Constant")) {
          Time session_constant;
          profile_input >> parameter >> session_constant;
          Ptr<RandomVariableStream> Session = mobility_profile->GetSession();
          Session = CreateObject<ConstantRandomVariable>();
          Session->SetAttribute("Constant", DoubleValue(session_constant.GetMinutes()));
        }
      }
      
      else if(parameter == "##end-profile") {
        cout << "Mobility Profile Created" << endl;
        cout << "Number AP: " << mobility_profile->GetNumberAp() << endl;
        userProfiles.Add(mobility_profile);  
        break;
      }
      }
    }
    
  }
  
  RngSeedManager::SetSeed(seed);
  RngSeedManager::SetRun(run);  

  AnnotatedTopologyReader topologyReader("", 1);
  topologyReader.SetFileName("/home/lleal/scripts-ndnSIM/" + topology);
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
  vector<Ptr<ListPositionAllocator> > positionAlloc(n_users);
  vector<int> profile(n_users);
  for (uint32_t i = 0; i < n_users; i++)
  {
    positionAlloc[i] = CreateObject<ListPositionAllocator>();

    profile[i] = rand() % userProfiles.GetN();

    // randomly select a user profile stored in MobilityProfilesContainer
    shared_ptr<ns3::ndn::MobilityProfile> current_profile = userProfiles.Get(profile[i]);
    cout << "Node " << i << ": ";
    vector<bool> routerPositions(n_routers, false);
    for (uint32_t j=0; j<current_profile->GetNumberAp(); j++) {
      uint32_t routerPos = rand() % n_routers;

      if (!routerPositions[routerPos]) {
        routerPositions[routerPos] = true;
        cout << routerPos << " ";
        positionAlloc[i]->Add(Vector(routerPos, 0, 0));
        p2p.Install(routerNodes.Get(routerPos), userNodes.Get(i));
      }
    }
    cout << endl;
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
  for (uint32_t i = 0; i < n_users; i++) {
    string prefix = "/prod" + to_string(i);
    ndn::StrategyChoiceHelper::InstallAll(prefix, "/localhost/nfd/strategy/best-route");
  }
  ndn::StrategyChoiceHelper::InstallAll("/hint", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::InstallAll("/vicinity", "/localhost/nfd/strategy/multicast");

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
  
  Ptr<UniformRandomVariable> initialization = CreateObject<UniformRandomVariable>();

  cout << "User" << endl;
  // User
  uint32_t producers = 0;
  for (uint32_t i = 0; i < n_users; i++) {
    Time publishTime = Minutes(initialization->GetValue(min_publish_time.GetMinutes(), max_publish_time.GetMinutes()));

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
  }

  for (uint32_t i = 0; i < n_users; i++) {
    MobilityHelper mobility;
    
    shared_ptr<ns3::ndn::MobilityProfile> current_profile = userProfiles.Get(profile[i]);
    for (uint32_t j=0; j<current_profile->GetNumberAp(); j++) {
      uint32_t routerPos = positionAlloc[i]->GetNext().x;
      
      cout << routerNodes.Get(routerPos) << " " << userNodes.Get(i) << endl;
      ndn::LinkControlHelper::FailLink(routerNodes.Get(routerPos), userNodes.Get(i));
    }

    //uint32_t pos = positionAlloc[i]->GetNext().x;
    //ndn::LinkControlHelper::UpLink(routerNodes.Get(pos), userNodes.Get(i));

    //cout << "Initialize positions of nodes" << endl;
    // Initialize positions of nodes
    mobility.SetPositionAllocator(positionAlloc[i]);
    
    mobility.SetMobilityModel("ns3::OnOffMobilityModel",
                             "Movement", PointerValue(current_profile->GetMovement()),
                              "Session", PointerValue(current_profile->GetSession()),
                              "PositionAllocator", PointerValue(positionAlloc[i]));

    mobility.Install(userNodes.Get(i));
  }
  
  ndn::GlobalRoutingHelper grHelper;
  grHelper.InstallAll();

  for (uint32_t i=0; i<catalog_size; i++) {
      cout << "Adding origin /prod" << to_string(i) << endl;
      grHelper.AddOrigin("/prod" + to_string(i), userNodes.Get(i));
  }
  
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
