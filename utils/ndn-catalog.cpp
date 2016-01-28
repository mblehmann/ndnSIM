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
  : m_catalog()
  , m_users() // added by prlanzarin
{
}

//TODO parametized constructor

//TODO setters &/|| getters for simulation parameters

NameService::~NameService()
{
}

void
NameService::publishContent(Name object)
{
  m_catalog.push_back(object);
}

/*
void NameService::setRequests(Name object)
{
  for (int i = 0; i < m_users.size(); i++) {
      Simulator::ScheduleiNow(&MobileUser::AddInterestObject, &m_users[i], &newObject, 3);
  }
}
*/

Name NameService::getLastObjectPublished()
{
  return m_catalog.back();
}

uint32_t 
NameService::getCatalogSize()
{
  return m_catalog.size();
}

uint32_t NameService::getNumberOfContents()
{
  return m_numberOfContents;
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


/**
 * Initializes the popularity vector.
 * added by prlanzarin
 */
void
NameService::initializePopularity(uint32_t numberOfObjects, float alpha)
{
	uint32_t sampleSize = 10000; // TODO this should be reviewed
	uint32_t contentObjectIdx;

	m_popularity.assign(numberOfObjects, 0.0);
	m_zipf.assign(numberOfObjects, 0.0);	

	initializeZipf(numberOfObjects, alpha);

	// Content popularity setup
	for (uint32_t i = 0; i < sampleSize; i++) {
		contentObjectIdx = nextZipf();
		m_popularity.at(contentObjectIdx) += 1;
	}
	
	// Distribuition of requests (percentage)
	for (contentObjectIdx = 0; contentObjectIdx < m_popularity.size(); contentObjectIdx++) {
		m_popularity.at(contentObjectIdx) = m_popularity.at(contentObjectIdx) / float(sampleSize);
	}
}

/**
 * Returns the popularity for a content by shuffling the popularity vector
 * and popping the last element.
 * added by prlanzarin
 */
double
NameService::nextContentPopularity()
{
	random_shuffle(m_popularity.begin(), m_popularity.end());
	double popularity = m_popularity.back();
    if(m_popularity.empty() == false)
        m_popularity.pop_back();
	return popularity;
}

/**
 * Initializes the zipf distribuition vector.
 * added by prlanzarin
 */
void 
NameService::initializeZipf(uint32_t numberOfObjects, float alpha)
{
  static double cnst = 0;          // Normalization constant
  double sums;
  uint32_t i;

  for (i = 1; i <= numberOfObjects; i++)
    cnst = cnst + (1.0 / pow((double) i, alpha));
  cnst = 1.0 / cnst;

	// Stores the map
  sums = 0;
  for (i = 1; i <= numberOfObjects; i++) {
    sums = sums + cnst / pow((double) i, alpha);
		m_zipf.push_back(sums);
  }
  return;
}

/**
 * Returns the mapped element of the next zipf distribuition value.
 * added by prlanzarin
 */
double
NameService::nextZipf()
{
  double z = 0, zipfv;

  // Pull a uniform random number (0 < z < 1)
  do {
	 // TODO use ns-3 randomizer
   // z = rand(0);
  }
  while ((z == 0) || (z == 1));

  // Map z to the value
  for (uint32_t i = 1; i <= m_numberOfContents; i++) {
    if (m_zipf.at(i) >= z) {
      zipfv = i;
      break;
    }
  }
  return zipfv;
}

/**
 * Initializes a normal distribuition of content sizes (stored in a vector) 
 * added by prlanzarin
 */
void
initializeContentSizes(uint32_t numberOfObjects) 
{
  
  return;
}

/**
 * Analog to nextPopularity(), returns the next content size by shuffling
 * the vector and popping the last element.
 * added by prlanzarin
 */
uint32_t
nextContentSize()
{

  return 0;
}


} // namespace ndn
} // namespace ns3
