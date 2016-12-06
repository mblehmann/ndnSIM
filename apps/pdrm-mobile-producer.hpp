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
 *
 * @author Matheus Lehmann <mblehmann@inf.ufrgs.br>
 * @author Lucas Leal <lsleal@inf.ufrgs.br>
 */

#ifndef PDRM_MOBILE_PRODUCER_H
#define PDRM_MOBILE_PRODUCER_H

#include "pdrm-producer.hpp"

#include "ns3/mobility-model.h"

using namespace std;

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A mobile user that can be a producer, provider, and consumer 
 */
class PDRMMobileProducer : public PDRMProducer {
public:
  static TypeId
  GetTypeId();

  // Constructor
  PDRMMobileProducer();
  virtual ~PDRMMobileProducer(){};

  // Finalize data structures and print collected statistics
  void
  EndGame();

  pair<uint32_t, double>
  GetPreferredLocation();  

  virtual void
  CourseChange(Ptr<const MobilityModel> model);

  virtual void
  Move(Ptr<const MobilityModel> model);

  virtual void
  Session(Ptr<const MobilityModel> model);

  typedef void (*MobilityEventCallback)(Ptr<App> app, bool isMoving, uint32_t homeNetwork, uint32_t position, Time session, Time movement, double availability);

protected:

  virtual void
  StartApplication();

  virtual void
  StopApplication();

protected:
  // Movement
  bool m_moving;
  Vector m_position;
  Time m_sessionPeriod;
  Time m_movementPeriod;
  Time m_lastMobilityEvent;

  // Own Information
  bool m_mobile;
  double m_userAvailability;
  uint32_t m_homeNetwork;
  map<uint32_t, Time> m_timeSpent;

  // Tracers
  TracedCallback<Ptr<App>, bool, uint32_t, uint32_t, Time, Time, double> m_mobilityEvent;
 
};

} // namespace ndn
} // namespace ns3

#endif // PDRM_MOBILE_PRODUCER_H


