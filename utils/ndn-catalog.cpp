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

NameService::NameService()
{
}

NameService::~NameService()
{
}

void
NameService::addUser(Ptr<Node> user)
{
  m_users.push_back(user);
}

vector<Ptr<Node>> NameService::getUsers()
{
  return m_users;
}

void
NameService::addRouter(Ptr<Node> router)
{
  m_routers.push_back(router);
}

vector<Ptr<Node>> NameService::getRouters()
{
  return m_routers;
}

void
NameService::setCatalogSize(uint32_t catalogSize)
{
  m_catalogSize = catalogSize;
}

void
NameService::initializePopularity(double alpha)
{
  m_alpha = alpha;

  m_base = 0;
  for (uint32_t k = 1; k <= m_catalogSize; k++)
  {
    m_popularityIndex.push_back(k);
    m_base += pow(k, -1*m_alpha);
  }
  random_shuffle(m_popularityIndex.begin(), m_popularityIndex.end());
}

double
NameService::getNextPopularity()
{
  uint32_t index = m_popularityIndex.back();
  m_popularityIndex.pop_back();

  return getContentPopularity(index);
}

double
NameService::getContentPopularity(uint32_t rank)
{
  return pow(rank, -1*m_alpha)/m_base;
}

/**
 * Initializes a normal distribuition of content sizes (stored in a vector) 
 * added by prlanzarin
 */
void
NameService::initializeObjectSize(double mean, double stddev)
{
  m_objectSize = CreateObject<NormalRandomVariable>();
  m_objectSize->SetAttribute("Mean", DoubleValue(mean));
  m_objectSize->SetAttribute("Variance", DoubleValue(stddev));
}

/**
 * Analog to nextPopularity(), returns the next content size by shuffling
 * the vector and popping the last element.
 * added by prlanzarin
 */
uint32_t
NameService::getNextObjectSize()
{
  return round(m_objectSize->GetValue());
}

} // namespace ndn
} // namespace ns3
