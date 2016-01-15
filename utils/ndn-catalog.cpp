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

#include <ndn-cxx/name.hpp>
#include <list>
#include <vector>

using namespace std;

namespace ns3 {
namespace ndn {
	
using ::ndn::Name;

NameService::NameService()
  : m_catalog()
  , m_users()
{
}

NameService::~NameService()
{
}

void NameService::publishContent(Name object)
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

uint32_t NameService::getCatalogSize()
{
  return m_catalog.size();
}

void NameService::addUser(Ptr<Node> user)
{
  m_users.push_back(user);
}

vector<Ptr<Node>> NameService::getUsers()
{
  return m_users;
}

} // namespace ndn
} // namespace ns3
