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

#ifndef NDN_NAME_SERVICE_H
#define NDN_NAME_SERVICE_H

#include "ns3/object.h"

#include "ns3/core-module.h"
#include "ns3/network-module.h"

#include <ndn-cxx/name.hpp>
#include <list>
#include <vector>

using namespace std;

namespace ns3 {
namespace ndn {
	
using ::ndn::Name;

struct objectProperties {
  double popularity;
  uint32_t size;
//  double lifetime;
};

class Catalog : public Object
{
public:
  Catalog();
  ~Catalog();
  
  void
  addUser(Ptr<Node> user);

  vector<Ptr<Node>>
  getUsers();

  void
  addRouter(Ptr<Node> router);

  vector<Ptr<Node>>
  getRouters();

  Time
  getMaxSimulationTime();

  void
  setMaxSimulationTime(Time maxSimulationTime);

  uint32_t
  getUserPopulationSize();

  void
  setUserPopulationSize(uint32_t userPopulationSize);

  void
  setPopularity(double alpha, uint32_t maxRequests);

  void 
  setObjectPopularityVariation(double stddev);

  void 
  setObjectSize(double mean, double stddev);

//  void
//  setObjectLifetime(double mean, double stddev);

  void
  initializeCatalog();

  void
  addObject(Name objectName, double popularity);

  objectProperties
  getObjectProperties(Name objectName);

  void
  removeObject(Name objectName);

  double
  getUserPopularity();

private: 
  void
  initializePopularity();
  
  void
  initializeObjectSize();

//  void
//  initializeObjectLifetime();

  double 
  getNextPopularity();

  double
  getPopularity(uint32_t rank);

  double
  getNextObjectPopularity(double popularity);

  uint32_t
  getNextObjectSize();

//  double
//  getNextObjectLifetime();

private:
  vector<Ptr<Node>> m_users;
  vector<Ptr<Node>> m_routers;

  vector<uint32_t> m_popularityIndex;

  Ptr<NormalRandomVariable> m_objectPopularity;
  Ptr<NormalRandomVariable> m_objectSize;
//  Ptr<NormalRandomVariable> m_objectLifetime;

  Time m_maxSimulationTime;
  uint32_t m_userPopulationSize;

  double m_maxRequests;
  double m_objectPopularityAlpha;
  double m_objectPopularityBase;
  double m_objectPopularityStddev;

  double m_objectSizeMean;
  double m_objectSizeStddev;

//  double m_objectLifetimeMean;
//  double m_objectLifetimeStddev;

  map<Name, objectProperties> m_objects;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_NAME_SERVICE_H
