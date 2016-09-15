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

#include "ndn-profile-container.hpp"

#include <algorithm>

namespace ns3 {
namespace ndn {

MobilityProfileContainer::MobilityProfileContainer() = default;

MobilityProfileContainer::MobilityProfileContainer(const MobilityProfileContainer& other)
{
  AddAll(other);
}

MobilityProfileContainer&
MobilityProfileContainer::operator= (const MobilityProfileContainer &other)
{
  m_profiles.clear();
  AddAll(other);

  return *this;
}

void
MobilityProfileContainer::AddAll(Ptr<MobilityProfileContainer> other)
{
  AddAll(*other);
}

void
MobilityProfileContainer::AddAll(const MobilityProfileContainer& other)
{
  if (this == &other) { // adding self to self, need to make a copy
    auto copyOfMobilityProfiles = other.m_profiles;
    m_profiles.insert(m_profiles.end(), copyOfMobilityProfiles.begin(), copyOfMobilityProfiles.end());
  }
  else {
    m_profiles.insert(m_profiles.end(), other.m_profiles.begin(), other.m_profiles.end());
  }
}

MobilityProfileContainer::Iterator
MobilityProfileContainer::Begin(void) const
{
  return m_profiles.begin();
}

MobilityProfileContainer::Iterator
MobilityProfileContainer::End(void) const
{
  return m_profiles.end();
}

uint32_t
MobilityProfileContainer::GetN(void) const
{
  return m_profiles.size();
}

void
MobilityProfileContainer::Add(shared_ptr<MobilityProfile> mobility_profile)
{
  m_profiles.push_back(mobility_profile);
}

shared_ptr<MobilityProfile>
MobilityProfileContainer::Get(size_t i) const
{
  return m_profiles.at(i);
}

} //ndn
} //ns3
