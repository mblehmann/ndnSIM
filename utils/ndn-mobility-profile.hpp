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

#ifndef NDN_MOBILITY_PROFILE_H
#define NDN_MOBILITY_PROFILE_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"

#include "ns3/mobility-module.h"

#include <ns3/ndnSIM/utils/ndn-catalog.hpp>

#include <iostream>
#include <fstream>

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include <stdint.h>
#include <vector>

#include "ns3/ptr.h"
#include "ns3/simple-ref-count.h"

namespace ns3 {
namespace ndn {

class MobilityProfile {
private:
  /* number of AP the user will travel through */
  uint32_t m_numberAP;

  /* parameters related to user movement */
  Ptr<RandomVariableStream> m_movement;

  /* paramenters related to user sessions */
  Ptr<RandomVariableStream> m_session; 
 
public:
  /**
   * Create an empty MobilityProfile.
   */
  MobilityProfile();

  // mutators
  void
  SetNumberAp(uint32_t numberAP);

  void
  SetMovementDistribution (Ptr<RandomVariableStream> movement);

  void
  SetSessionDistribution(Ptr<RandomVariableStream> session);
  
  // accessors
  uint32_t
  GetNumberAp() {
    return m_numberAP; 
  }
  
  Ptr<RandomVariableStream>
  GetMovement() {
    return m_movement;
  }
  
  Ptr<RandomVariableStream>
  GetSession() {
    return m_session;
  }

};


} // namespace ndn
} // namespace ns3

#endif /* NDN_MOBILITY_PROFILE_H */
