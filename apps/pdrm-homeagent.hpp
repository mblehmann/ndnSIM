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

#ifndef PDRM_HOMEAGENT_H
#define PDRM_HOMEAGENT_H

#include "ndn-app.hpp"

#include <utils/pdrm-catalog.hpp>
#include <utils/pdrm-global.hpp>

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A simple Interest-sink applia simple Interest-sink application
 *
 * A simple Interest-sink applia simple Interest-sink application,
 * which replying every incoming Interest with Data packet with a specified
 * size and name same as in Interest.cation, which replying every incoming Interest
 * with Data packet with a specified size and name same as in Interest.
 */
class PDRMHomeAgent : public App {
public:
  static TypeId
  GetTypeId(void);

  PDRMHomeAgent();

  // inherited from NdnApp
  virtual void
  OnTimeout(Name chunk);

  virtual void
  OnInterest(shared_ptr<const Interest> interest);

  virtual void
  Register(Name producerPrefix);

  virtual void
  Unregister(Name producerPrefix);

//  typedef void (*ServedDataCallback)(Ptr<App> app, Name object, string role);

protected:
  // inherited from Application base class.
  virtual void
  StartApplication(); // Called at time specified by Start

  virtual void
  StopApplication(); // Called at time specified by Stop

protected:
  Name m_homeAgentPrefix;
  Name m_registerPrefix;
  Name m_unregisterPrefix;

  set<Name> m_unavailableProducers;
  map<Name, vector<Name> > m_storedObjects;
  map<Name, map<Name, EventId> > m_retxEvent;
  Ptr<UniformRandomVariable> m_rand; // nonce generator
  Time m_interestLifeTime;

  Ptr<PDRMCatalog> m_catalog;
  Ptr<PDRMGlobal> m_global;

//  TracedCallback<Ptr<App>, Name, string> m_servedData;
};

} // namespace ndn
} // namespace ns3

#endif // PDRM_HOMEAGENT_H
