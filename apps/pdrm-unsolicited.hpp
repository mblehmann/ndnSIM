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

#ifndef PDRM_UNSOLICITED_H
#define PDRM_UNSOLICITED_H

#include "pdrm-mobile-producer.hpp"

using namespace std;

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A mobile user that can be a producer, provider, and consumer 
 */
class PDRMUnsolicited : public PDRMMobileProducer {
public:
  static TypeId
  GetTypeId();

  // Constructor
  PDRMUnsolicited();

  // Finalize data structures and print collected statistics
  void
  EndGame();

  void
  OnTimeoutUnsolicitedData(Name object);

  void
  OnInterest(shared_ptr<const Interest> interest);

  void
  PushUnsolicitedData();

  void
  SendUnsolicitedData(Name object);

  virtual void
  CourseChange(Ptr<const MobilityModel> model);

  virtual void
  UnsolicitedMove(Ptr<const MobilityModel> model);

  typedef void (*PushedUnsolicitedDataCallback)(Ptr<App> app, Name object);
  typedef void (*PushedUnsolicitedObjectCallback)(Ptr<App> app, Name object, bool isPushed, bool isTimeout);

protected:

  virtual void
  StartApplication();

  virtual void
  StopApplication();

private:
  // Strategy (vicinity, hint, and replication)
  uint32_t m_objectsToPush;
  vector<Name> m_activeRequests;

  Name m_unsolicitedPrefix;

  Time m_idleRequest;
  map<Name, EventId> m_timeoutUnsolicitedData;

  TracedCallback<Ptr<App>, Name> m_pushedUnsolicitedData;
  TracedCallback<Ptr<App>, Name, bool, bool> m_pushedUnsolicitedObject;
};

} // namespace ndn
} // namespace ns3

#endif // PDRM_UNSOLICITED_H

