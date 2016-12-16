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
 *
 * @author Matheus Lehmann <mblehmann@inf.ufrgs.br>
 * @author Lucas Leal <lsleal@inf.ufrgs.br>
 */

#ifndef PDRM_PROVIDER_H
#define PDRM_PROVIDER_H

#include "pdrm-consumer.hpp"

using namespace std;

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A mobile user that can be a producer, provider, and consumer 
 */
class PDRMProvider : public PDRMConsumer {
public:
  static TypeId
  GetTypeId();

  // Constructor
  PDRMProvider();

  // Finalize data structures and print collected statistics
  void
  EndGame();

  virtual void
  OnInterest(shared_ptr<const Interest> interest);

  virtual void
  AnnouncePrefix(Name prefix);

  virtual void
  UnannouncePrefix(Name prefix);

  virtual void
  UpdateNetwork();

  virtual void
  StoreObject(Name object);

  virtual void
  DeleteObject();

  typedef void (*ServedDataCallback)(Ptr<App> app, Name object);
  typedef void (*AnnouncedPrefixCallback)(Ptr<App> app, Name prefix, bool isAnnouncing);

protected:

  virtual void
  StartApplication();

  virtual void
  StopApplication();

protected:
  set<Name> m_announcedPrefixes;
  EventId m_updateNetwork;

  uint32_t m_virtualPayloadSize;
  Time m_freshness;

  // Providing content
  vector<Name> m_storage;
  uint32_t m_storageSize;
  uint32_t m_storedObjects;
  uint32_t m_circularIndex;

  // Producer ID
  uint32_t m_signature;
  Name m_keyLocator;

  TracedCallback<Ptr<App>, Name> m_servedData;
  TracedCallback<Ptr<App>, Name, bool> m_announcedPrefix;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_MOBILE_USER_H


