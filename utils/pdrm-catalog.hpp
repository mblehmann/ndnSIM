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

#ifndef PDRM_CATALOG_H
#define PDRM_CATALOG_H

#include "ns3/object.h"

#include "ns3/core-module.h"

#include <ndn-cxx/name.hpp>

#include "ns3/random-variable-stream.h"

using namespace std;

namespace ns3 {
namespace ndn {
	
using ::ndn::Name;

struct ContentObject {
  Name name;
  uint32_t size;
  double availability;
  uint32_t locality;
};

class PDRMCatalog : public Object
{
public:
  PDRMCatalog();
  ~PDRMCatalog();

  // locality
  void
  setLocality(uint32_t domains, double locality);

  void
  initializeLocalityCatalog(uint32_t size, double alpha);

  ContentObject
  getLocalityObjectRequest(uint32_t domain);

  double
  getLocalityRequestProbability(Name object, uint32_t domain);

  // default
  void
  setObjectSize(uint32_t objectSize);

  uint32_t
  getCatalogSize();

  void
  initializeCatalog(uint32_t size, double alpha);

  void
  addObject(Name object, uint32_t index);

  void
  addObject(Name object);

  ContentObject
  getObject(Name object);

  ContentObject
  getObjectRequest();

  uint32_t
  getObjectPopularity(Name object);

  double
  getRequestProbability(Name object);

private:
  map<uint32_t, ContentObject> m_catalog;
  map<Name, uint32_t> m_popularity;

  Ptr<ZipfRandomVariable> m_popularityDistribution;
  Ptr<UniformRandomVariable> m_rand;

  uint32_t m_objectSize;
  uint32_t m_catalogSize;

  double m_totalProbability;
  double m_alpha;

  // locality
  uint32_t m_domains;
  double m_locality;
};

} // namespace ndn
} // namespace ns3

#endif // PDRM_CATALOG_H
