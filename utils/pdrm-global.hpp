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

#ifndef PDRM_GLOBAL_H
#define PDRM_GLOBAL_H

#include "ns3/object.h"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "helper/ndn-global-routing-helper.hpp"

using namespace std;

namespace ns3 {
namespace ndn {
	
class PDRMGlobal : public Object
{
public:
  PDRMGlobal();
  ~PDRMGlobal();
  
  void
  setRouters(vector<Ptr<Node> > routers);

  Ptr<Node>
  getRouter(int index);

  void
  setMaxSimulationTime(Time maxSimulationTime);

  Time
  getMaxSimulationTime();

  void
  setGlobalRoutingHelper(ndn::GlobalRoutingHelper ndnGlobalRoutingHelper);

  ndn::GlobalRoutingHelper
  getGlobalRoutingHelper();

private:
  vector<Ptr<Node>> m_routers;
  Time m_maxSimulationTime;
  ndn::GlobalRoutingHelper m_ndnGlobalRoutingHelper;
};

} // namespace ndn
} // namespace ns3

#endif // PDRM_GLOBAL_H
