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

#include "ndn-catalog.hpp"
#include "ns3/random-variable-stream.h"

//TODO remove temporary imports
#include "ns3/log.h"

#include <ndn-cxx/name.hpp>

#include <list>
#include <vector>
#include <algorithm>
#include <math.h>
#include <random>

using namespace std;


namespace ns3 {
namespace ndn {
	
using ::ndn::Name;

Catalog::Catalog()
{
}

Catalog::~Catalog()
{
}

void
Catalog::addUser(Ptr<Node> user)
{
  m_users.push_back(user);
}

vector<Ptr<Node>> Catalog::getUsers()
{
  return m_users;
}

void
Catalog::addRouter(Ptr<Node> router)
{
  m_routers.push_back(router);
}

vector<Ptr<Node>> Catalog::getRouters()
{
  return m_routers;
}

Time
Catalog::getMaxSimulationTime()
{
  return m_maxSimulationTime;
}

void
Catalog::setMaxSimulationTime(Time maxSimulationTime)
{
  m_maxSimulationTime = maxSimulationTime;
}

uint32_t
Catalog::getUserPopulationSize()
{
  return m_userPopulationSize;
}

void
Catalog::setUserPopulationSize(uint32_t userPopulationSize)
{
  m_userPopulationSize = userPopulationSize;
}

void
Catalog::setPopularity(double alpha, uint32_t maxRequests)
{
  m_objectPopularityAlpha = alpha;
  m_maxRequests = maxRequests;
}

void
Catalog::setObjectPopularityVariation(double stddev)
{
  m_objectPopularityStddev = stddev;
}

void
Catalog::setObjectSize(double mean, double stddev)
{
  m_objectSizeMean = mean;
  m_objectSizeStddev = stddev;
}

//void
//Catalog::setObjectLifetime(double mean, double stddev)
//{
//  m_objectLifetimeMean = mean;
//  m_objectLifetimeStddev = stddev;
//}

void
Catalog::initializeCatalog()
{
  initializePopularity();
  initializeObjectSize();
//  initializeObjectLifetime();
}

void
Catalog::initializePopularity()
{
  m_objectPopularityBase = 0;
  for (uint32_t k = 1; k <= m_userPopulationSize; k++)
  {
    m_popularityIndex.push_back(k);
    m_objectPopularityBase += pow(k, -1*m_objectPopularityAlpha);
  }
  random_shuffle(m_popularityIndex.begin(), m_popularityIndex.end());
}

void
Catalog::initializeObjectSize()
{
  m_objectSize = CreateObject<NormalRandomVariable>();
  m_objectSize->SetAttribute("Mean", DoubleValue(m_objectSizeMean));
  m_objectSize->SetAttribute("Variance", DoubleValue(m_objectSizeStddev));
}

//void
//Catalog::initializeObjectLifetime()
//{
//  m_objectLifetime = CreateObject<NormalRandomVariable>();
//  m_objectLifetime->SetAttribute("Mean", DoubleValue(m_objectLifetimeMean));
//  m_objectLifetime->SetAttribute("Variance", DoubleValue(m_objectLifetimeStddev));
//}

double
Catalog::getUserPopularity()
{
  return getNextPopularity();
}

double
Catalog::getNextPopularity()
{
  uint32_t index = m_popularityIndex.back();
  m_popularityIndex.pop_back();

  double mostPopular = getPopularity(1);
  double popularity = getPopularity(index);
  double scaleFactor = m_maxRequests/mostPopular;

  //cout << "#1: " << mostPopular << " / #" << index << ": " << popularity << " / requests: " << m_maxRequests << " / scale: " << scaleFactor << endl;  

  return (popularity * scaleFactor) / m_userPopulationSize;
}

double
Catalog::getPopularity(uint32_t rank)
{
  return pow(rank, -1*m_objectPopularityAlpha)/m_objectPopularityBase;
}

double
Catalog::getNextObjectPopularity(double popularity)
{
  m_objectPopularity = CreateObject<NormalRandomVariable>();
  m_objectPopularity->SetAttribute("Mean", DoubleValue(popularity));
  m_objectPopularity->SetAttribute("Variance", DoubleValue(m_objectPopularityStddev));
  return m_objectPopularity->GetValue();
}

/**
 * Analog to nextPopularity(), returns the next content size by shuffling
 * the vector and popping the last element.
 */
uint32_t
Catalog::getNextObjectSize()
{
  return round(m_objectSize->GetValue());
}

//double
//Catalog::getNextObjectLifetime()
//{
//  return m_objectLifetime->GetValue();
//}

void
Catalog::addObject(Name objectName, double popularity)
{
  objectProperties properties;

  properties.popularity = getNextObjectPopularity(popularity);
  properties.size = getNextObjectSize();
//  properties.lifetime = getNextObjectLifetime();

  m_objects[objectName] = properties;
}

objectProperties
Catalog::getObjectProperties(Name objectName)
{
  return m_objects[objectName];
}

void
Catalog::removeObject(Name objectName)
{
  m_objects.erase(objectName);
}

} // namespace ndn
} // namespace ns3
