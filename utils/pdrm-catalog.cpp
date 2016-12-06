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

#include "pdrm-catalog.hpp"

#include "ns3/log.h"

using namespace std;

namespace ns3 {
namespace ndn {
	
using ::ndn::Name;

PDRMCatalog::PDRMCatalog()
  : m_rand(CreateObject<UniformRandomVariable>())
{
}

PDRMCatalog::~PDRMCatalog()
{
}

void
PDRMCatalog::setObjectSize(uint32_t objectSize)
{
  m_objectSize = objectSize;
}

uint32_t
PDRMCatalog::getCatalogSize()
{
  return m_catalogSize;
}

void
PDRMCatalog::initializeCatalog(uint32_t size, double alpha)
{
  m_popularityDistribution = CreateObject<ZipfRandomVariable>();
  m_popularityDistribution->SetAttribute("N", IntegerValue(size));
  m_popularityDistribution->SetAttribute("Alpha", DoubleValue(alpha));

  m_catalogSize = size;
}

void
PDRMCatalog::addObject(Name object, uint32_t index)
{
  ContentObject producedObject;
  producedObject.name = object;
  producedObject.size = m_objectSize;
  producedObject.availability = m_rand->GetValue(0, 1);

  m_catalog[index] = producedObject;
  m_popularity[object] = index;
}

void
PDRMCatalog::addObject(Name object)
{
  uint32_t objectIndex = m_rand->GetValue(0, m_catalog.size());
  addObject(object, objectIndex);
}

ContentObject
PDRMCatalog::getObject(Name object)
{
  uint32_t objectIndex = getObjectPopularity(object);
  return m_catalog[objectIndex];
}

ContentObject
PDRMCatalog::getObjectRequest()
{
  uint32_t objectIndex = m_popularityDistribution->GetInteger() - 1;
  return m_catalog[objectIndex];
}

uint32_t
PDRMCatalog::getObjectPopularity(Name object)
{
  return m_popularity[object];
}

} // namespace ndn
} // namespace ns3
