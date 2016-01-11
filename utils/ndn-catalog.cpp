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

#include "ndn-catalog.h"

#include <list>
#include <ns3/assert.h>

Catalog::Catalog()
  : m_catalog()
{
}

Catalog::~Catalog()
{
}

void Catalog::publishContent(Name object)
{
  m_catalog.push_back(object);
}

Name Catalog::getLastObjectPublished()
{
  return m_catalog.back();
}

uint32_t Catalog::getCatalogSize()
{
  return m_catalog.size();
}

}
