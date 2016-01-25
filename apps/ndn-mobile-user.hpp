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
#include "ns3/ndnSIM/model/ndn-app-face.hpp"

#include "ndn-consumer.hpp"
#include "ndn-producer.hpp"

#include "ns3/random-variable-stream.h"
#include "ns3/traced-value.h"
#include "util/time.hpp"

#include <utils/ndn-catalog.hpp>

#include <vector>
#include <list>

using namespace std;

namespace ns3 {
namespace ndn {

const time::milliseconds DEFAULT_VICINITY_TIMER = time::milliseconds(30000); // 30 seconds

const uint32_t DEFAULT_REPLICATION_DEGREE = 1; // 1 replica

/**
 * @ingroup ndn-apps
 * @brief A mobile consumer that requests multiple objects with varied number of chunks.
 */
class MobileUser : public App {
public:
  static TypeId
  GetTypeId();

  /* Default Constructor */
  MobileUser();

  // Retransmission Control
  virtual void
  SetRetxTimer(Time retxTimer);

  Time
  GetRetxTimer() const;

  virtual void
  CheckRetxTimeout();

  // Setters
  virtual void
  setVicinityTimer(Time vicinityTimer);

  virtual void
  setReplicationDegree(uint32_t replicationDegree);

  // Packet Handlers
  virtual void
  OnData(shared_ptr<const Data> contentObject);

  virtual void
  OnInterest(shared_ptr<const Interest> interest);

  virtual void
  OnTimeout(Name objectName);

  virtual void
  OnAnnouncement(shared_ptr<const Announcement> announcement);

  virtual void
  OnHint(shared_ptr<const Hint> hint);

  virtual void
  OnVicinity(shared_ptr<const Vicinity> vicinity);

  virtual void
  OnVicinityData(shared_ptr<const VicinityData> vicinityData);

  // Consumer
  virtual void
  AddInterestObject(Name objectName, uint32_t chunks);

  virtual void
  ConcludeObjectDownload(Name objectName);

  void 
  SendInterestPacket();

  virtual void
  WillSendOutInterest(Name objectName);

  // Provider
  virtual void
  AnnounceContent();

  virtual void
  AnnounceContent(Name object);

  // Producer
  virtual void
  PublishContent();

  virtual Name
  createContent();

  virtual void
  advertiseContent(Name object);

  virtual void
  discoverVicinity(Name object);

  virtual void
  PushContent();

  virtual int
  selectDevice();

  virtual void
  sendContent(int deviceID, Name object);

public:
  typedef void (*AnnouncementTraceCallback)(shared_ptr<const Announcement>, Ptr<App>, shared_ptr<Face>);
  typedef void (*VicinityTraceCallback)(shared_ptr<const Vicinity>, Ptr<App>, shared_ptr<Face>);
  typedef void (*HintTraceCallback)(shared_ptr<const Hint>, Ptr<App>, shared_ptr<Face>);

protected:

  virtual void
  StartApplication();

  virtual void
  StopApplication();

  virtual void
  ScheduleNextPacket();

private:
  EventId m_sendEvent; ///< @brief EventId of pending "send packet" event
  Time m_retxTimer;    ///< @brief Currently estimated retransmission timer
  EventId m_retxEvent; ///< @brief Event to check whether or not retransmission should be performed

  Ptr<RttEstimator> m_rtt; ///< @brief RTT estimator
  Time m_interestLifeTime; ///< \brief LifeTime for interest packet

  Name m_prefix;
  Name m_postfix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;

  uint32_t m_signature;
  Name m_keyLocator;

  // Consumer Interest queue
  vector<Name> m_interestQueue;
  
  // Pending objects (sent, but not received)
  list<Name> m_pendingObjects;

  // Number of pending interests
  uint32_t m_pendingInterests;

  // Window size (traffic control)
  uint32_t m_windowSize;

  // Catalog of the content in the network
  Ptr<NameService> m_nameService;

  // Retransmission queue
  vector<Name> m_retxNames;

  // Objects that the user is providing
  vector<Name> m_providedObjects;

  // Structures for retransmission 
  map<Name, Time> m_nameTimeouts;
 
  map<Name, Time> m_nameLastDelay;
  map<Name, Time> m_nameFullDelay;
  map<Name, uint32_t> m_nameRetxCounts;

  TracedCallback<Ptr<App>, Name, Time, int32_t> m_lastRetransmittedInterestDataDelay;
  TracedCallback<Ptr<App>, Name, Time, uint32_t, int32_t> m_firstInterestDataDelay;
 
  SequenceNumber32 m_chunksRetrieved;
  map<Name, SequenceNumber32> m_chunkOrder;

  // Content generated by this producer
  vector<Name> m_generatedContent;

  // Vicinity learned by this producer
  vector<int> m_vicinity;

  // Queue of objects to push
  queue<Name> m_pushedObjects;

  // Period for learning the vicinity
  Time m_vicinityTimer;

  // Replication degree of the content
  uint32_t m_replicationDegree;

  Ptr<UniformRandomVariable> m_rand; ///< @brief nonce generator

  TracedCallback<shared_ptr<const Announcement>, Ptr<App>, shared_ptr<Face>>
    m_transmittedAnnouncements; ///< @brief App-level trace of transmitted Announcement

  TracedCallback<shared_ptr<const Vicinity>, Ptr<App>, shared_ptr<Face>>
    m_transmittedVicinities; ///< @brief App-level trace of transmitted Vicinity

  TracedCallback<shared_ptr<const Hint>, Ptr<App>, shared_ptr<Face>>
    m_transmittedHints; ///< @brief App-level trace of transmitted Hint

  TracedCallback<shared_ptr<const VicinityData>, Ptr<App>, shared_ptr<Face>>
    m_transmittedVicinityDatas; ///< @brief App-level trace of transmitted Vicinity Data

};

} // namespace ndn
} // namespace ns3

#endif // NDN_MOBILE_USER_H


