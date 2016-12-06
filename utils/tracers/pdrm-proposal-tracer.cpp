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

#include "pdrm-proposal-tracer.hpp"

#include "ns3/config.h"
#include "ns3/names.h"

#include "apps/ndn-app.hpp"
#include "ns3/node-list.h"
#include "ns3/log.h"

#include <boost/lexical_cast.hpp>

#include <fstream>

NS_LOG_COMPONENT_DEFINE("ndn.PDRMProposalTracer");

namespace ns3 {
namespace ndn {

static std::list<std::tuple<shared_ptr<std::ostream>, std::list<Ptr<PDRMProposalTracer>>>>
  g_tracers;

void
PDRMProposalTracer::Destroy()
{
  g_tracers.clear();
}

void
PDRMProposalTracer::InstallAll(const std::string& file)
{
  using namespace boost;
  using namespace std;

  std::list<Ptr<PDRMProposalTracer>> tracers;
  shared_ptr<std::ostream> outputStream;
  if (file != "-") {
    shared_ptr<std::ofstream> os(new std::ofstream());
    os->open(file.c_str(), std::ios_base::out | std::ios_base::trunc);

    if (!os->is_open()) {
      NS_LOG_ERROR("File " << file << " cannot be opened for writing. Tracing disabled");
      return;
    }

    outputStream = os;
  }
  else {
    outputStream = shared_ptr<std::ostream>(&std::cout, std::bind([]{}));
  }

  for (NodeList::Iterator node = NodeList::Begin(); node != NodeList::End(); node++) {
    Ptr<PDRMProposalTracer> trace = Install(*node, outputStream);
    tracers.push_back(trace);
  }

  if (tracers.size() > 0) {
    // *m_l3RateTrace << "# "; // not necessary for R's read.table
    tracers.front()->PrintHeader(*outputStream);
  }

  g_tracers.push_back(std::make_tuple(outputStream, tracers));
}

void
PDRMProposalTracer::Install(const NodeContainer& nodes, const std::string& file)
{
  using namespace boost;
  using namespace std;

  std::list<Ptr<PDRMProposalTracer>> tracers;
  shared_ptr<std::ostream> outputStream;
  if (file != "-") {
    shared_ptr<std::ofstream> os(new std::ofstream());
    os->open(file.c_str(), std::ios_base::out | std::ios_base::trunc);

    if (!os->is_open()) {
      NS_LOG_ERROR("File " << file << " cannot be opened for writing. Tracing disabled");
      return;
    }

    outputStream = os;
  }
  else {
    outputStream = shared_ptr<std::ostream>(&std::cout, std::bind([]{}));
  }

  for (NodeContainer::Iterator node = nodes.Begin(); node != nodes.End(); node++) {
    Ptr<PDRMProposalTracer> trace = Install(*node, outputStream);
    tracers.push_back(trace);
  }

  if (tracers.size() > 0) {
    // *m_l3RateTrace << "# "; // not necessary for R's read.table
    tracers.front()->PrintHeader(*outputStream);
  }

  g_tracers.push_back(std::make_tuple(outputStream, tracers));
}

void
PDRMProposalTracer::Install(Ptr<Node> node, const std::string& file)
{
  using namespace boost;
  using namespace std;

  std::list<Ptr<PDRMProposalTracer>> tracers;
  shared_ptr<std::ostream> outputStream;
  if (file != "-") {
    shared_ptr<std::ofstream> os(new std::ofstream());
    os->open(file.c_str(), std::ios_base::out | std::ios_base::trunc);

    if (!os->is_open()) {
      NS_LOG_ERROR("File " << file << " cannot be opened for writing. Tracing disabled");
      return;
    }

    outputStream = os;
  }
  else {
    outputStream = shared_ptr<std::ostream>(&std::cout, std::bind([]{}));
  }

  Ptr<PDRMProposalTracer> trace = Install(node, outputStream);
  tracers.push_back(trace);

  if (tracers.size() > 0) {
    // *m_l3RateTrace << "# "; // not necessary for R's read.table
    tracers.front()->PrintHeader(*outputStream);
  }

  g_tracers.push_back(std::make_tuple(outputStream, tracers));
}

Ptr<PDRMProposalTracer>
PDRMProposalTracer::Install(Ptr<Node> node, shared_ptr<std::ostream> outputStream)
{
  NS_LOG_DEBUG("Node: " << node->GetId());

  Ptr<PDRMProposalTracer> trace = Create<PDRMProposalTracer>(outputStream, node);

  return trace;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

PDRMProposalTracer::PDRMProposalTracer(shared_ptr<std::ostream> os, Ptr<Node> node)
  : m_nodePtr(node)
  , m_os(os)
{
  m_node = boost::lexical_cast<std::string>(m_nodePtr->GetId());

  Connect();

  std::string name = Names::FindName(node);
  if (!name.empty()) {
    m_node = name;
  }
}

PDRMProposalTracer::PDRMProposalTracer(shared_ptr<std::ostream> os, const std::string& node)
  : m_node(node)
  , m_os(os)
{
  Connect();
}

PDRMProposalTracer::~PDRMProposalTracer(){};

void
PDRMProposalTracer::Connect()
{
  Config::ConnectWithoutContext("/NodeList/" + m_node + "/ApplicationList/*/PushedUnsolicitedData",
                                MakeCallback(&PDRMProposalTracer::PushedUnsolicitedData, this));

  Config::ConnectWithoutContext("/NodeList/" + m_node + "/ApplicationList/*/PushedUnsolicitedObject",
                                MakeCallback(&PDRMProposalTracer::PushedUnsolicitedObject, this));

  Config::ConnectWithoutContext("/NodeList/" + m_node + "/ApplicationList/*/InterceptedInterest",
                                MakeCallback(&PDRMProposalTracer::InterceptedInterest, this));

  Config::ConnectWithoutContext("/NodeList/" + m_node + "/ApplicationList/*/ReceivedHint",
                                MakeCallback(&PDRMProposalTracer::ReceivedHint, this));

  Config::ConnectWithoutContext("/NodeList/" + m_node + "/ApplicationList/*/ReceivedVicinityData",
                                MakeCallback(&PDRMProposalTracer::ReceivedVicinityData, this));

  Config::ConnectWithoutContext("/NodeList/" + m_node + "/ApplicationList/*/ReplicatedContent",
                                MakeCallback(&PDRMProposalTracer::ReplicatedContent, this));

  Config::ConnectWithoutContext("/NodeList/" + m_node + "/ApplicationList/*/ProbedVicinity",
                                MakeCallback(&PDRMProposalTracer::ProbedVicinity, this));

  Config::ConnectWithoutContext("/NodeList/" + m_node + "/ApplicationList/*/SelectedDevice",
                                MakeCallback(&PDRMProposalTracer::SelectedDevice, this));

  Config::ConnectWithoutContext("/NodeList/" + m_node + "/ApplicationList/*/HintedContent",
                                MakeCallback(&PDRMProposalTracer::HintedContent, this));

}

void
PDRMProposalTracer::PrintHeader(std::ostream& os) const
{
  os << "PushedUnsolicitedData\tTime\tNode\tAppId\tObject\n";
  os << "PushedUnsolicitedObject\tTime\tNode\tAppId\tObject\tIsPushed\tIsTimeout\n";
  os << "InterceptedInterest\tTime\tNode\tAppId\tObject\tIsStored\tIsTimeout\tIsSent\n";
  os << "ReceivedHint\tTime\tNode\tAppId\tObject\tIsAccepted\n";
  os << "ReceivedVicinityData\tTime\tNode\tAppId\tObject\tNodeId\tCurrentPosition\tAvailability\tIsInterested\n";
  os << "ReplicatedContent\tTime\tNode\tAppId\tObject\n";
  os << "ProbedVicinity\tTime\tNode\tAppId\tObject\tVicinitySize\n";
  os << "SelectedDevice\tTime\tNode\tAppId\tObject\tIsSatisfied\tIsHinted\tNodeId\tProducerAvailability\n";
  os << "HintedContent\tTime\tNode\tAppId\tObject\tDeviceId\n";
}

void
PDRMProposalTracer::PushedUnsolicitedData(Ptr<App> app, Name object)
{
  *m_os << "PushedUnsolicitedData" << "\t" << Simulator::Now().ToDouble(Time::S) << "\t" << m_node << "\t"
        << app->GetId() << "\t" << object << "\n";
}

void
PDRMProposalTracer::PushedUnsolicitedObject(Ptr<App> app, Name object, bool isPushed, bool isTimeout)
{
  *m_os << "PushedUnsolicitedObject" << "\t" << Simulator::Now().ToDouble(Time::S) << "\t" << m_node << "\t"
        << app->GetId() << "\t" << object << "\t" << isPushed << "\t" << isTimeout << "\n";
}

void
PDRMProposalTracer::InterceptedInterest(Ptr<App> app, Name object, bool isStored, bool isTimeout, bool isSent)
{
  *m_os << "InterceptedInterest" << "\t" << Simulator::Now().ToDouble(Time::S) << "\t" << m_node << "\t"
        << app->GetId() << "\t" << object << "\t" << isStored << "\t" << isTimeout << "\t" << isSent << "\n";
}

void
PDRMProposalTracer::ReceivedHint(Ptr<App> app, Name object, bool isAccepted)
{
  *m_os << "ReceivedHint" << "\t" << Simulator::Now().ToDouble(Time::S) << "\t" << m_node << "\t"
        << app->GetId() << "\t" << object << "\t" << isAccepted << "\n";
}

void
PDRMProposalTracer::ReceivedVicinityData(Ptr<App> app, Name object, int32_t nodeId, int32_t currentPosition, double availability, bool isInterested)
{
  *m_os << "ReceivedVicinityData" << "\t" << Simulator::Now().ToDouble(Time::S) << "\t" << m_node << "\t"
        << app->GetId() << "\t" << object << "\t"
        << nodeId << "\t" << currentPosition << "\t" << availability << "\t" << isInterested << "\n";
}

void
PDRMProposalTracer::ReplicatedContent(Ptr<App> app, Name object)
{
  *m_os << "ReplicatedContent" << "\t" << Simulator::Now().ToDouble(Time::S) << "\t" << m_node << "\t"
        << app->GetId() << "\t" << object << "\n";
}

void
PDRMProposalTracer::ProbedVicinity(Ptr<App> app, Name object, int32_t vicinitySize)
{
  *m_os << "ProbedVicinity" << "\t" << Simulator::Now().ToDouble(Time::S) << "\t" << m_node << "\t"
        << app->GetId() << "\t" << object << "\t" << vicinitySize << "\n";
}

void
PDRMProposalTracer::SelectedDevice(Ptr<App> app, Name object, bool isSatisfied, bool isHinted, int nodeId, double producerAvailability)
{
  *m_os << "SelectedDevice" << "\t" << Simulator::Now().ToDouble(Time::S) << "\t" << m_node << "\t"
        << app->GetId() << "\t" << object << "\t" << isSatisfied << "\t" << isHinted << "\t"
        << nodeId << "\t" << producerAvailability << "\n";
}

void
PDRMProposalTracer::HintedContent(Ptr<App> app, Name object, int deviceId)
{
  *m_os << "HintedContent" << "\t" << Simulator::Now().ToDouble(Time::S) << "\t" << m_node << "\t"
        << app->GetId() << "\t" << object << "\t" << deviceId << "\n";
}

} // namespace ndn
} // namespace ns3
