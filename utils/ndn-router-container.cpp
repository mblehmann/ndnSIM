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

#include "ndn-router-container.hpp"

#include <algorithm>

namespace ns3 {
namespace ndn {

RouterContainer::RouterContainer() = default;

RouterContainer::RouterContainer(const RouterContainer& other)
{
  AddAll(other);
}

RouterContainer&
RouterContainer::operator= (const RouterContainer &other)
{
  m_routers.clear();
  AddAll(other);

  return *this;
}

void
RouterContainer::AddAll(Ptr<RouterContainer> other)
{
  AddAll(*other);
}

void
RouterContainer::AddAll(const RouterContainer& other)
{
  if (this == &other) { // adding self to self, need to make a copy
    auto copyOfRouters = other.m_routers;
    m_routers.insert(m_routers.end(), copyOfRouters.begin(), copyOfRouters.end());
  }
  else {
    m_routers.insert(m_routers.end(), other.m_routers.begin(), other.m_routers.end());
  }
}

RouterContainer::Iterator
RouterContainer::Begin(void) const
{
  return m_routers.begin();
}

RouterContainer::Iterator
RouterContainer::End(void) const
{
  return m_routers.end();
}

uint32_t
RouterContainer::GetN(void) const
{
  return m_routers.size();
}

void
RouterContainer::Add(Ptr<Node> router)
{
  m_routers.push_back(router);
}

Ptr<Node>
RouterContainer::Get(size_t i) const
{
  return m_routers.at(i);
}

} //ndn
} //ns3
