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

#include <list>

#include "ns3/object.h"
#include "ns3/random-variable.h"

#include "ndn-cxx/name.hpp"

using namespace std;

class NameService : public ns3::Object
{
public:
  NameService();
  ~NameService();
  
  void publishContent(Name object);
  void setRequests(Name object);

  Name getLastObjectPublished();
  uint32_t getCatalogSize();

private:
  list<Name> m_catalog;
  //list<Nodes> m_users;
  
};

#endif // NDN_NAME_SERVICE_H
