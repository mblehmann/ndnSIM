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

#include "pdrm-mobile-tracer.hpp"

#include "ns3/config.h"
#include "ns3/names.h"

#include "apps/ndn-app.hpp"
#include "ns3/node-list.h"
#include "ns3/log.h"

#include <boost/lexical_cast.hpp>

#include <fstream>

NS_LOG_COMPONENT_DEFINE("ndn.PDRMMobileTracer");

namespace ns3 {
namespace ndn {

static std::list<std::tuple<shared_ptr<std::ostream>, std::list<Ptr<PDRMMobileTracer>>>>
  g_tracers;

void
PDRMMobileTracer::Destroy()
{
  g_tracers.clear();
}

void
PDRMMobileTracer::InstallAll(const std::string& file)
{
  using namespace boost;
  using namespace std;

  std::list<Ptr<PDRMMobileTracer>> tracers;
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
    Ptr<PDRMMobileTracer> trace = Install(*node, outputStream);
    tracers.push_back(trace);
  }

  if (tracers.size() > 0) {
    // *m_l3RateTrace << "# "; // not necessary for R's read.table
    tracers.front()->PrintHeader(*outputStream);
  }

  g_tracers.push_back(std::make_tuple(outputStream, tracers));
}

void
PDRMMobileTracer::Install(const NodeContainer& nodes, const std::string& file)
{
  using namespace boost;
  using namespace std;

  std::list<Ptr<PDRMMobileTracer>> tracers;
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
    Ptr<PDRMMobileTracer> trace = Install(*node, outputStream);
    tracers.push_back(trace);
  }

  if (tracers.size() > 0) {
    // *m_l3RateTrace << "# "; // not necessary for R's read.table
    tracers.front()->PrintHeader(*outputStream);
  }

  g_tracers.push_back(std::make_tuple(outputStream, tracers));
}

void
PDRMMobileTracer::Install(Ptr<Node> node, const std::string& file)
{
  using namespace boost;
  using namespace std;

  std::list<Ptr<PDRMMobileTracer>> tracers;
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

  Ptr<PDRMMobileTracer> trace = Install(node, outputStream);
  tracers.push_back(trace);

  if (tracers.size() > 0) {
    // *m_l3RateTrace << "# "; // not necessary for R's read.table
    tracers.front()->PrintHeader(*outputStream);
  }

  g_tracers.push_back(std::make_tuple(outputStream, tracers));
}

Ptr<PDRMMobileTracer>
PDRMMobileTracer::Install(Ptr<Node> node, shared_ptr<std::ostream> outputStream)
{
  NS_LOG_DEBUG("Node: " << node->GetId());

  Ptr<PDRMMobileTracer> trace = Create<PDRMMobileTracer>(outputStream, node);

  return trace;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

PDRMMobileTracer::PDRMMobileTracer(shared_ptr<std::ostream> os, Ptr<Node> node)
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

PDRMMobileTracer::PDRMMobileTracer(shared_ptr<std::ostream> os, const std::string& node)
  : m_node(node)
  , m_os(os)
{
  Connect();
}

PDRMMobileTracer::~PDRMMobileTracer(){};

void
PDRMMobileTracer::Connect()
{
  Config::ConnectWithoutContext("/NodeList/" + m_node + "/ApplicationList/*/MobilityEvent",
                                MakeCallback(&PDRMMobileTracer::MobilityEvent, this));
}

void
PDRMMobileTracer::PrintHeader(std::ostream& os) const
{
  os << "MobilityEvent\tTime\tNode\tAppId\tIsMoving\tHomeNetwork\tPosition\tSession\tMovement\tAvailability\n";
}

void
PDRMMobileTracer::MobilityEvent(Ptr<App> app, bool isMoving, uint32_t homeNetwork, uint32_t position, Time session, Time movement, double availability)
{
  *m_os << "MobilityEvent" << "\t" << Simulator::Now().ToDouble(Time::S) << "\t" << m_node << "\t"
        << app->GetId() << "\t" << isMoving << "\t" << homeNetwork << "\t" << position << "\t" 
        << session.ToDouble(Time::S) << "\t" << movement.ToDouble(Time::S) << "\t" << availability << "\n";
}

} // namespace ndn
} // namespace ns3
