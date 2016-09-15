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

#include "ndn-mobility-profile.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.MobilityProfile");

using namespace std;

namespace ns3 {
namespace ndn {

/* MobilityProfiles */

/**
 * Create an empty MobilityProfile.
 */
MobilityProfile::MobilityProfile() = default;

// mutators
void
MobilityProfile::SetNumberAp(uint32_t nap) {
  numberAP = nap;
}

void
MobilityProfile::SetMovementDistribution (string movement_dist ) {
  movement_distribution = movement_dist;
}

void
MobilityProfile::SetMovementMin(Time mov_min) {
  movement_min = mov_min;
}

void
MobilityProfile::SetMovementMax(Time mov_max) {
  movement_max = mov_max;
}

void
MobilityProfile::SetMovementConstant(Time mov_const) {
  movement_constant = mov_const;
}

void
MobilityProfile::SetSessionDistribution(string sess_dist) {
  session_distribution = sess_dist;
}

void
MobilityProfile::SetSessionMean(double mean) {
  session_mean = mean;
}

void
MobilityProfile::SetSessionShape(double shape) {
  session_shape = shape;
}

void
MobilityProfile::SetSessionBound(double bound) {
  session_bound = bound;;
}

void
MobilityProfile::SetSessionConstant(Time ses_const) {
  session_constant = ses_const;;
}

/* MobilityProfileContainer */

} // namespace ndn
} // namespace ns3
