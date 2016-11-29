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

#ifndef NDN_MOBILE_USER_H
#define NDN_MOBILE_USER_H
#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "ns3/ndnSIM/model/ndn-app-face.hpp"

#include "ndn-consumer.hpp"
#include "ndn-producer.hpp"

#include "ns3/random-variable-stream.h"
#include "ns3/traced-value.h"
#include "util/time.hpp"

#include "ns3/mobility-model.h"

#include <utils/ndn-catalog.hpp>

#include <vector>
#include <set>
#include <queue>

using namespace std;

namespace ns3 {
namespace ndn {

/** 
 * @brief A wrap used by the device ranking mechanism containing the relevant User information.
 * Currently, we can store the node ID, location, availability, and interest.
 * This information is used in a mapping content -> UserInformation
 */
class UserInformation {
public:
  // Constructor
  UserInformation(uint32_t, uint32_t, uint32_t, bool);

  // Geters
  uint32_t
  GetNodeId() { return m_nodeId; }

  uint32_t
  GetLocation() { return m_location; }

  uint32_t
  GetAvailability() { return m_availability; }

  bool
  GetInterested() { return m_interested; }

  bool
  operator==(const UserInformation& a) { return this->m_nodeId == a.m_nodeId; }
 
private:
  uint32_t m_nodeId;
  uint32_t m_location;
  uint32_t m_availability;
  bool m_interested;
};

/**
 * @ingroup ndn-apps
 * @brief A mobile user that can be a producer, provider, and consumer 
 */
class MobileUser : public App {
public:
  static TypeId
  GetTypeId();

  // Constructor
  MobileUser();

  // Finalize data structures and print collected statistics
  void
  EndGame();

  /* 
   * Retransmission Control methods.
   * It calculates a moving average based on past requests
   */
  virtual void
  SetRetxTimer(Time retxTimer);

  Time
  GetRetxTimer() const;

  virtual void
  CheckRetxTimeout();

  virtual void
  OnTimeout(Name objectName);

  /* 
   * Packet/Event Handler
   * There are two main packets: Data and Interest.
   * A Data packet can be actual Data or Vicinity Data
   * An Interest packet can be actual Interest, Hint, or Vicinity
   */
  virtual void
  OnInterest(shared_ptr<const Interest> interest);

  virtual void
  RespondInterest(shared_ptr<const Interest> interest);

  virtual void
  RespondHint(shared_ptr<const Interest> interest);

  virtual void
  RespondVicinity(shared_ptr<const Interest> interest);

  virtual void
  OnData(shared_ptr<const Data> contentObject);

  virtual void
  RespondData(shared_ptr<const Data> contentObject);

  virtual void
  RespondVicinityData(shared_ptr<const Data> contentObject);

//  virtual void
//  respondAnnouncement(shared_ptr<const Announcement> announcement);

  /*
   * Consumer actions
   * A consumer adds interest to an object to signalize it will start downloading
   * There are three actions for sending interest packets that manage data structures and local cache
   * Lastly, the consumer concludes a download
   */
  virtual void
  FoundObject(Name objectName);

  virtual void
  StartObjectDownload(Name objectName);

  virtual void
  ScheduleNextInterestPacket();

  void 
  SendInterestPacket();

  virtual void
  WillSendOutInterest(Name objectName);

  virtual void
  ConcludeObjectDownload(Name objectName);

  /*
   * Provider actions
   * A provider can announce and unannounce a given content
   * Besides, it manages the cache
   */
  virtual void
  AnnounceContent(Name object);

  virtual void
  UnannounceContent(Name object);

  virtual void
  StoreObject(Name object, uint32_t size);

  virtual void
  DeleteObject(int32_t amount);

  /*
   * Producer actions
   * A producer publishes content and advertise it
   */
  virtual void
  ProduceContent();

  virtual Name
  PublishContent();

//  virtual void
//  ExpireContent(Name expiredObject);

  /*
   * Strategy actions
   * A user probes, sorts, and selects a device of the vicinity
   * A user also pushes content to others using a hinting scheme
   */
  virtual void
  ReplicateContent(Name object);

  virtual void
  ProbeVicinity(Name object);

  virtual void
  PushContent(Name objectName);

  virtual void
  PushToSelectedDevices(Name objectName);

  virtual void
  PushToRandomDevices(Name objectName);

  virtual UserInformation
  SelectBestDevice(uint32_t location, bool include, double availability);

  virtual void
  HintContent(int deviceID, Name object);

  /*
   * Mobility actions
   * A user can start movement or begin a session
   * The course change notifies a change of state between movement and session
   */
  void
  CourseChange(Ptr<const MobilityModel> model);

  virtual void
  Move(Ptr<const MobilityModel> model);

  virtual void
  Session(Ptr<const MobilityModel> model);

protected:

  virtual void
  StartApplication();

  virtual void
  StopApplication();

private:
  // Sending Interest packets
  EventId m_sendEvent;
  uint32_t m_windowSize;
  queue<Name> m_chunkRequestQueue;
  set<Name> m_pendingChunks;

  // Objects management
  map<Name, uint32_t> m_downloadedChunks;
  map<Name, uint32_t> m_objectSizes;
  set<Name> m_interestingObjects;
  set<Name> m_requestedObjects;
  set<Name> m_downloadedObjects;

  // RTT and lifetime
  Ptr<RttEstimator> m_rtt;
  Time m_interestLifeTime;

  // Produced content
  Time m_produceTime;
  Name m_prefix;
  Name m_postfix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;
  double m_producerPopularity;
  Name m_lastProducedObject;
  set<Name> m_producedObjects;

  // Producer ID
  uint32_t m_signature;
  Name m_keyLocator;

  // Movement
  bool m_movingRequesting;
  bool m_movingProducing;
  bool m_movingReplicating;
  bool m_moving;
  Vector m_position;
  Time m_sessionPeriod;
  Time m_movementPeriod;
  Time m_lastMovementEvent;

  // Providing content
  uint32_t m_cacheSize;
  uint32_t m_usedCache;
  map<Name, uint32_t> m_providedObjects;

  // Content catalog and global structures
  Ptr<Catalog> m_catalog;
  Ptr<UniformRandomVariable> m_rand;

  // Strategy (vicinity, hint, and replication)
  vector<UserInformation> m_vicinity;
  vector<UserInformation> m_consumers;
  Time m_vicinityTimer;
  Time m_hintTimer;
  uint32_t m_vicinitySize;

  // Content Placement Policies structures
  double m_userAvailability;
  uint32_t m_placementPolicy;
  uint32_t m_homeNetwork;

  // Retransmission
  Time m_retxTimer;
  EventId m_retxEvent;
  queue<Name> m_retxChunksQueue;
  map<Name, Time> m_chunkTimeouts;
 
  map<Name, Time> m_chunkLastDelay;
  map<Name, Time> m_chunkFullDelay;
  map<Name, uint32_t> m_chunkRetxCounts;

  map<Name, SequenceNumber32> m_chunkOrder;

  // Tracers
  TracedCallback<Ptr<App>, Name, Time, int32_t> m_lastRetransmittedInterestDataDelay;
  TracedCallback<Ptr<App>, Name, Time, uint32_t, int32_t> m_firstInterestDataDelay;
  TracedCallback<Ptr<App>, Name> m_servedData;
 
};

} // namespace ndn
} // namespace ns3

#endif // NDN_MOBILE_USER_H


