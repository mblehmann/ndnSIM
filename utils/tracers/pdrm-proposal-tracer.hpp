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

#ifndef PDRM_PROPOSAL_TRACER_H
#define PDRM_PROPOSAL_TRACER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/ptr.h"
#include "ns3/simple-ref-count.h"
#include <ns3/node-container.h>

namespace ns3 {

class Node;
class Packet;

namespace ndn {

class App;

/**
 * @ingroup ndn-tracers
 * @brief Tracer to obtain application-level delays
 */
class PDRMProposalTracer : public SimpleRefCount<PDRMProposalTracer> {
public:
  /**
   * @brief Helper method to install tracers on all simulation nodes
   *
   * @param file File to which traces will be written.  If filename is -, then std::out is used
   *
   */
  static void
  InstallAll(const std::string& file);

  /**
   * @brief Helper method to install tracers on the selected simulation nodes
   *
   * @param nodes Nodes on which to install tracer
   * @param file File to which traces will be written.  If filename is -, then std::out is used
   *
   */
  static void
  Install(const NodeContainer& nodes, const std::string& file);

  /**
   * @brief Helper method to install tracers on a specific simulation node
   *
   * @param nodes Nodes on which to install tracer
   * @param file File to which traces will be written.  If filename is -, then std::out is used
   * @param averagingPeriod How often data will be written into the trace file (default, every half
   *        second)
   */
  static void
  Install(Ptr<Node> node, const std::string& file);

  /**
   * @brief Helper method to install tracers on a specific simulation node
   *
   * @param nodes Nodes on which to install tracer
   * @param outputStream Smart pointer to a stream
   * @param averagingPeriod How often data will be written into the trace file (default, every half
   *        second)
   *
   * @returns a tuple of reference to output stream and list of tracers.
   *          !!! Attention !!! This tuple needs to be preserved for the lifetime of simulation,
   *          otherwise SEGFAULTs are inevitable
   */
  static Ptr<PDRMProposalTracer>
  Install(Ptr<Node> node, shared_ptr<std::ostream> outputStream);

  /**
   * @brief Explicit request to remove all statically created tracers
   *
   * This method can be helpful if simulation scenario contains several independent run,
   * or if it is desired to do a postprocessing of the resulting data
   */
  static void
  Destroy();

  /**
   * @brief Trace constructor that attaches to all applications on the node using node's pointer
   * @param os    reference to the output stream
   * @param node  pointer to the node
   */
  PDRMProposalTracer(shared_ptr<std::ostream> os, Ptr<Node> node);

  /**
   * @brief Trace constructor that attaches to all applications on the node using node's name
   * @param os        reference to the output stream
   * @param nodeName  name of the node registered using Names::Add
   */
  PDRMProposalTracer(shared_ptr<std::ostream> os, const std::string& node);

  /**
   * @brief Destructor
   */
  ~PDRMProposalTracer();

  /**
   * @brief Print head of the trace (e.g., for post-processing)
   *
   * @param os reference to output stream
   */
  void
  PrintHeader(std::ostream& os) const;

private:
  void
  Connect();

  void
  PushedUnsolicitedData(Ptr<App> app, Name object);

  void
  PushedUnsolicitedObject(Ptr<App> app, Name object, bool isPushed, bool isTimeout);

  void
  InterceptedInterest(Ptr<App> app, Name object, bool isStored, bool isTimeout, bool isSent);

  void
  ReceivedHint(Ptr<App> app, Name object, bool isAccepted);

  void
  ReceivedVicinityData(Ptr<App> app, Name object, int32_t nodeId, int32_t currentPosition, double availability, bool isInterested);

  void
  ReplicatedContent(Ptr<App> app, Name object);

  void
  ProbedVicinity(Ptr<App> app, Name object, int32_t vicinitySize);

  void
  SelectedDevice(Ptr<App> app, Name object, bool isSatisfied, bool isHinted, int nodeId, double producerAvailability);

  void
  HintedContent(Ptr<App> app, Name object, int deviceId);

private:
  std::string m_node;
  Ptr<Node> m_nodePtr;

  shared_ptr<std::ostream> m_os;
};

} // namespace ndn
} // namespace ns3

#endif // PDRM_PROPOSAL_TRACER_H
