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

#ifndef NDN_PROBEPRODUCER_H
#define NDN_PROBEPRODUCER_H

#include "ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/ndnSIM-module.h"

#include "ns3/mobility-model.h"

#include "ns3/nstime.h"
#include "ns3/ptr.h"

#include "helper/ndn-link-control-helper.hpp"
#include <utils/ndn-catalog.hpp>
#include <vector>

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A simple Interest-sink applia simple Interest-sink application
 *
 * A simple Interest-sink applia simple Interest-sink application,
 * which replying every incoming Interest with Data packet with a specified
 * size and name same as in Interest.cation, which replying every incoming Interest
 * with Data packet with a specified size and name same as in Interest.
 */
class ProbeProducer : public App {
public:
  static TypeId
  GetTypeId(void);

  ProbeProducer();

  // inherited from NdnApp
  virtual void
  OnInterest(shared_ptr<const Interest> interest);

protected:
  // inherited from Application base class.
  virtual void
  StartApplication(); // Called at time specified by Start

  virtual void
  StopApplication(); // Called at time specified by Stop

  void
  CourseChange(Ptr<const MobilityModel> model);

  //void
  //onErrorFetch(uint32_t errorCode, const std::string& errorMsg);

  //void
  //afterFetchedFibEnumerationInformation(const ConstBufferPtr& dataset);

protected:
  Name m_prefix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;

  uint32_t m_signature;
  Name m_keyLocator;

  Ptr<Catalog> m_catalog;

};

} // namespace ndn
} // namespace ns3

#endif // NDN_PROBEPRODUCER_H