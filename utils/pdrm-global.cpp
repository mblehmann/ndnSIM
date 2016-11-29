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

#include "pdrm-global.hpp"
#include "ns3/random-variable-stream.h"

#include "ns3/log.h"

#include <vector>

using namespace std;

namespace ns3 {
namespace ndn {
	
PDRMGlobal::PDRMGlobal()
{
}

PDRMGlobal::~PDRMGlobal()
{
}

void
PDRMGlobal::setRouters(vector<Ptr<Node> > routers)
{
  m_routers = routers;
}

Ptr<Node>
PDRMGlobal::getRouter(int index)
{
  return m_routers[index];
}

void
PDRMGlobal::setMaxSimulationTime(Time maxSimulationTime)
{
  m_maxSimulationTime = maxSimulationTime;
}

Time
PDRMGlobal::getMaxSimulationTime()
{
  return m_maxSimulationTime;
}

void
PDRMGlobal::setGlobalRoutingHelper(ndn::GlobalRoutingHelper ndnGlobalRoutingHelper)
{
  m_ndnGlobalRoutingHelper = ndnGlobalRoutingHelper;
}

ndn::GlobalRoutingHelper
PDRMGlobal::getGlobalRoutingHelper()
{
  return m_ndnGlobalRoutingHelper;
}

} // namespace ndn
} // namespace ns3
