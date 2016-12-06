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

#include "pdrm-mobile-producer.hpp"

#include "helper/ndn-link-control-helper.hpp"
#include "helper/ndn-global-routing-helper.hpp"

using namespace std;

NS_LOG_COMPONENT_DEFINE("ndn.PDRMMobileProducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(PDRMMobileProducer);

TypeId
PDRMMobileProducer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::PDRMMobileProducer")
      .SetGroupName("Ndn")
      .SetParent<PDRMProducer>()
      .AddConstructor<PDRMMobileProducer>()

      .AddAttribute("Mobile",
                    "Sets whether the producer is mobile or not (avoids problem with the mobility model)",
                    BooleanValue(true),
                    MakeBooleanAccessor(&PDRMMobileProducer::m_mobile),
                    MakeBooleanChecker())

      .AddTraceSource("MobilityEvent",
                      "Register movement and session of the mobile",
                      MakeTraceSourceAccessor(&PDRMMobileProducer::m_mobilityEvent),
                      "ns3::ndn::PDRMMobileProducer::MobilityEventCallback");

  return tid;
}

PDRMMobileProducer::PDRMMobileProducer()
{
}

/**
 *
 */
void
PDRMMobileProducer::StartApplication() 
{
  // initialize mobility and the signals for events  
  Ptr<MobilityModel> mob = this->GetNode()->GetObject<MobilityModel>();
  mob->TraceConnectWithoutContext("CourseChange", MakeCallback(&PDRMMobileProducer::CourseChange, this));

  // initialize the position
  m_moving = false;
  m_position = mob->GetPosition();
  ndn::LinkControlHelper::UpLink(this->GetNode(), m_global->getRouter(m_position.x));

  // Set Availability and Interested attributes for each user
  m_sessionPeriod = Time("0s");
  m_movementPeriod = Time("0s");
  m_lastMobilityEvent = Simulator::Now();

  m_userAvailability = 0;
  m_homeNetwork = m_position.x;

  NS_LOG_INFO("Node " << GetNode()->GetId() << " initial position at router " << m_position.x);

  PDRMProducer::StartApplication();
}


/**
 *
 */
void
PDRMMobileProducer::StopApplication()
{
  PDRMProducer::StopApplication();
}

void
PDRMMobileProducer::EndGame()
{
}

pair<uint32_t, double>
PDRMMobileProducer::GetPreferredLocation()
{
  NS_LOG_FUNCTION_NOARGS();
  pair<uint32_t, Time> bestLocation = make_pair(-1, Time("0s"));
  for (map<uint32_t, Time>::iterator it = m_timeSpent.begin(); it != m_timeSpent.end(); ++it)
  {
    if (it->second > bestLocation.second)
    {
      bestLocation.first = it->first;
      bestLocation.second = it->second;
    }
  }
  pair<uint32_t, double> preferredLocation = make_pair(bestLocation.first, bestLocation.second.GetSeconds() / (m_sessionPeriod.GetSeconds() + 0.001));
  return preferredLocation;
}

/**
 *
 */
void
PDRMMobileProducer::Move(Ptr<const MobilityModel> model)
{
  NS_LOG_FUNCTION_NOARGS();
  m_moving = true;
  if (m_timeSpent.count(m_position.x) == 0)
    m_timeSpent[m_position.x] = Time("0s");

  Time lastSession = Simulator::Now() - m_lastMobilityEvent;
  m_sessionPeriod += lastSession;
  m_timeSpent[m_position.x] += lastSession;

  ndn::LinkControlHelper::FailLink(this->GetNode(), m_global->getRouter(m_position.x));
  ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::PrintFIBs();

  m_lastMobilityEvent = Simulator::Now();
  m_userAvailability = m_sessionPeriod.GetSeconds() / (m_sessionPeriod.GetSeconds() + m_movementPeriod.GetSeconds() + 0.001);
  m_mobilityEvent(this, m_moving, m_homeNetwork, m_position.x, m_sessionPeriod, m_movementPeriod, m_userAvailability);
}

/**
 *
 */
void
PDRMMobileProducer::Session(Ptr<const MobilityModel> model)
{
  NS_LOG_FUNCTION_NOARGS();
  m_moving = false;
  Time lastMovement = Simulator::Now() - m_lastMobilityEvent;
  m_movementPeriod += lastMovement;

  m_position = model->GetPosition();
  ndn::LinkControlHelper::UpLink(this->GetNode(), m_global->getRouter(m_position.x));
  ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::PrintFIBs();

  m_lastMobilityEvent = Simulator::Now();
  m_userAvailability = m_sessionPeriod.GetSeconds() / (m_sessionPeriod.GetSeconds() + m_movementPeriod.GetSeconds() + 0.001);
  m_mobilityEvent(this, m_moving, m_homeNetwork, m_position.x, m_sessionPeriod, m_movementPeriod, m_userAvailability);
}

void
PDRMMobileProducer::CourseChange(Ptr<const MobilityModel> model)
{
  if (!m_mobile)
    return;

  if (m_moving)
    Session(model);
  else
    Move(model);

  NS_LOG_INFO(m_position.x);
}

} // namespace ndn
} // namespace ns3


