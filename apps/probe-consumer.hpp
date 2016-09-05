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

#ifndef NDN_CONSUMER_H
#define NDN_CONSUMER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-app.hpp"

#include "ns3/random-variable-stream.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"

#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/utils/ndn-rtt-estimator.hpp"
#include "ns3/ndnSIM/utils/ndn-fw-hop-count-tag.hpp"

#include <set>
#include <map>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * \brief NDN application for sending out Interest packets
 */
class ProbeConsumer : public App {
public:
  static TypeId
  GetTypeId();

  /**
   * \brief Default constructor
   * Sets up randomizer function and packet sequence number
   */
  ProbeConsumer();
  virtual ~ProbeConsumer(){};

  // From App
  virtual void
  OnData(shared_ptr<const Data> contentObject);

  /**
   * @brief Actually send packet
   */
  void
  SendPacket();

public:
  typedef void (*LastRetransmittedInterestDataDelayCallback)(Ptr<App> app, uint32_t seqno, Time delay, int32_t hopCount);
  typedef void (*FirstInterestDataDelayCallback)(Ptr<App> app, uint32_t seqno, Time delay, uint32_t retxCount, int32_t hopCount);

protected:
  // from App
  virtual void
  StartApplication();

  virtual void
  StopApplication();

  /**
   * \brief Constructs the Interest packet and sends it using a callback to the underlying NDN
   * protocol
   */
  virtual void
  ScheduleNextPacket();

protected:
  Ptr<UniformRandomVariable> m_rand; ///< @brief nonce generator

  uint32_t m_seq;      ///< @brief currently requested sequence number
  EventId m_sendEvent; ///< @brief EventId of pending "send packet" event
  double m_frequency; // Frequency of interest packets (in hertz)

  Name m_interestName;     ///< \brief NDN Name of the Interest (use Name)

  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */, int32_t /*hop count*/>
    m_lastRetransmittedInterestDataDelay;
  TracedCallback<Ptr<App> /* app */, uint32_t /* seqno */, Time /* delay */,
                 uint32_t /*retx count*/, int32_t /*hop count*/> m_firstInterestDataDelay;

  /// @endcond
};

} // namespace ndn
} // namespace ns3

#endif
