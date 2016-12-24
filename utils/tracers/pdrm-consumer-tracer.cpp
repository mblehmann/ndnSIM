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

#include "pdrm-consumer-tracer.hpp"

#include "ns3/config.h"
#include "ns3/names.h"

#include "apps/ndn-app.hpp"
#include "ns3/node-list.h"
#include "ns3/log.h"

#include <boost/lexical_cast.hpp>

#include <fstream>

NS_LOG_COMPONENT_DEFINE("ndn.PDRMConsumerTracer");

namespace ns3 {
namespace ndn {

static std::list<std::tuple<shared_ptr<std::ostream>, std::list<Ptr<PDRMConsumerTracer>>>>
  g_tracers;

void
PDRMConsumerTracer::Destroy()
{
  g_tracers.clear();
}

void
PDRMConsumerTracer::InstallAll(const std::string& file)
{
  using namespace boost;
  using namespace std;

  std::list<Ptr<PDRMConsumerTracer>> tracers;
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
    Ptr<PDRMConsumerTracer> trace = Install(*node, outputStream);
    tracers.push_back(trace);
  }

  if (tracers.size() > 0) {
    // *m_l3RateTrace << "# "; // not necessary for R's read.table
    tracers.front()->PrintHeader(*outputStream);
  }

  g_tracers.push_back(std::make_tuple(outputStream, tracers));
}

void
PDRMConsumerTracer::Install(const NodeContainer& nodes, const std::string& file)
{
  using namespace boost;
  using namespace std;

  std::list<Ptr<PDRMConsumerTracer>> tracers;
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
    Ptr<PDRMConsumerTracer> trace = Install(*node, outputStream);
    tracers.push_back(trace);
  }

  if (tracers.size() > 0) {
    // *m_l3RateTrace << "# "; // not necessary for R's read.table
    tracers.front()->PrintHeader(*outputStream);
  }

  g_tracers.push_back(std::make_tuple(outputStream, tracers));
}

void
PDRMConsumerTracer::Install(Ptr<Node> node, const std::string& file)
{
  using namespace boost;
  using namespace std;

  std::list<Ptr<PDRMConsumerTracer>> tracers;
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

  Ptr<PDRMConsumerTracer> trace = Install(node, outputStream);
  tracers.push_back(trace);

  if (tracers.size() > 0) {
    // *m_l3RateTrace << "# "; // not necessary for R's read.table
    tracers.front()->PrintHeader(*outputStream);
  }

  g_tracers.push_back(std::make_tuple(outputStream, tracers));
}

Ptr<PDRMConsumerTracer>
PDRMConsumerTracer::Install(Ptr<Node> node, shared_ptr<std::ostream> outputStream)
{
  NS_LOG_DEBUG("Node: " << node->GetId());

  Ptr<PDRMConsumerTracer> trace = Create<PDRMConsumerTracer>(outputStream, node);

  return trace;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

PDRMConsumerTracer::PDRMConsumerTracer(shared_ptr<std::ostream> os, Ptr<Node> node)
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

PDRMConsumerTracer::PDRMConsumerTracer(shared_ptr<std::ostream> os, const std::string& node)
  : m_node(node)
  , m_os(os)
{
  Connect();
}

PDRMConsumerTracer::~PDRMConsumerTracer(){};

void
PDRMConsumerTracer::Connect()
{
  Config::ConnectWithoutContext("/NodeList/" + m_node + "/ApplicationList/*/ChunkRetrievalDelay",
                                MakeCallback(&PDRMConsumerTracer::ChunkRetrievalDelay, this));

  Config::ConnectWithoutContext("/NodeList/" + m_node + "/ApplicationList/*/ObjectDownloadTime",
                                MakeCallback(&PDRMConsumerTracer::ObjectDownloadTime, this));

  Config::ConnectWithoutContext("/NodeList/" + m_node + "/ApplicationList/*/ChunkFailedDelay",
                                MakeCallback(&PDRMConsumerTracer::ChunkFailedDelay, this));

  Config::ConnectWithoutContext("/NodeList/" + m_node + "/ApplicationList/*/ObjectFailedDownload",
                                MakeCallback(&PDRMConsumerTracer::ObjectFailedDownload, this));
}

void
PDRMConsumerTracer::PrintHeader(std::ostream& os) const
{
  os << "ChunkDelay\tTime\tNode\tAppId\tChunk\tOrder\tTotalDelay\tLastDelay\tRequestCount\tHopCount\n";
  os << "DownloadTime\tTime\tNode\tAppId\tObject\tElapsedTime\tRequests\n";
  os << "ChunkFailed\tTime\tNode\tAppId\tChunk\tOrder\tTotalDelay\tLastDelay\tRequestCount\tHopCount\n";
  os << "FailedDownload\tTime\tNode\tAppId\tObject\tElapsedTime\tRequests\n";
}

void
PDRMConsumerTracer::ChunkRetrievalDelay(Ptr<App> app, Name chunk, uint32_t order, Time totalDelay, Time lastDelay, uint32_t requestCount, uint32_t hopCount)
{
  *m_os << "ChunkDelay" << "\t" << Simulator::Now().ToDouble(Time::S) << "\t" << m_node << "\t"
        << app->GetId() << "\t" << chunk << "\t" << order << "\t"
        << totalDelay.ToDouble(Time::S) << "\t" << lastDelay.ToDouble(Time::S) << "\t"
        << requestCount << "\t" << hopCount << "\n";
}

void
PDRMConsumerTracer::ObjectDownloadTime(Ptr<App> app, Name object, Time download, uint32_t requests)
{
  *m_os << "DownloadTime" << "\t" << Simulator::Now().ToDouble(Time::S) << "\t" << m_node << "\t"
        << app->GetId() << "\t" << object << "\t" 
        << download.ToDouble(Time::S) << "\t" << requests << "\n";
}

void
PDRMConsumerTracer::ChunkFailedDelay(Ptr<App> app, Name chunk, uint32_t order, Time totalDelay, Time lastDelay, uint32_t requestCount, uint32_t hopCount)
{
  *m_os << "ChunkFailed" << "\t" << Simulator::Now().ToDouble(Time::S) << "\t" << m_node << "\t"
        << app->GetId() << "\t" << chunk << "\t" << order << "\t"
        << totalDelay.ToDouble(Time::S) << "\t" << lastDelay.ToDouble(Time::S) << "\t"
        << requestCount << "\t" << hopCount << "\n";
}

void
PDRMConsumerTracer::ObjectFailedDownload(Ptr<App> app, Name object, Time download, uint32_t requests)
{
  *m_os << "FailedDownload" << "\t" << Simulator::Now().ToDouble(Time::S) << "\t" << m_node << "\t"
        << app->GetId() << "\t" << object << "\t" 
        << download.ToDouble(Time::S) << "\t" << requests << "\n";
}

} // namespace ndn
} // namespace ns3
