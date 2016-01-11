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
 */

#ifndef NDN_MOBILE_USER_H
#define NDN_MOBILE_USER_H
#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-consumer.hpp"
#include "ns3/traced-value.h"

#include <list>

using namespace std;

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A mobile consumer that requests multiple objects with varied number of chunks.
 */
class MobileUser : public Consumer {
public:
  static TypeId
  GetTypeId();

  /* Default Constructor */
  MobileUser();

  // From App
  virtual void
  OnData(shared_ptr<const Data> contentObject);

  virtual void
  OnTimeout(Name objectName);
  
  void
  SendPacket();

  virtual void
  WillSendOutInterest(Name objectName);

  // New methods
  virtual void
  AddInterestObject(Name objectName, uint32_t chunks);

  virtual void
  ConcludeObjectDownload(Name objectName);

protected:
  /**
   * \brief Constructs the Interest packet and sends it using a callback to the underlying NDN
   * protocol
   */
  virtual void
  ScheduleNextPacket();

private:
  list<Name> m_interestQueue;
  uint32_t m_windowsSize;

  // These are the current used variables in ndn-consumer. But we have to change them to the interest queue above.
  //uint32_t m_seq;      ///< @brief currently requested sequence number
  //uint32_t m_seqMax;   ///< @brief maximum number of sequence number
  //Name m_interestName;     ///< \brief NDN Name of the Interest (use Name)

};

} // namespace ndn
} // namespace ns3

#endif // NDN_MOBILE_USER_H


