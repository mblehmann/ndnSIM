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

#include "ns3/mobility-model.h"

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

  // Constructor
  MobileUser();

  // Retransmission Control
  virtual void
  SetRetxTimer(Time retxTimer);

  Time
  GetRetxTimer() const;

  virtual void
  CheckRetxTimeout();

  // Packet/Event Handlers
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
  void 
  SendInterestPacket();

  virtual void
  WillSendOutInterest(Name objectName);

  virtual void
  ScheduleNextInterestPacket();

  virtual void
  AddInterestObject(Name objectName, uint32_t chunks);

  virtual void
  ConcludeObjectDownload(Name objectName);

  // Provider
  virtual void
  AnnounceContent();

  virtual void
  AnnounceContent(Name object);

  // Producer
  virtual void
  PublishContent();

  virtual Name
  CreateContentName();

  virtual void
  AdvertiseContent(Name object, uint32_t chunks);

  // Strategy
  virtual void
  DiscoverVicinity(Name object);

  virtual void
  PushContent(Name objectName, uint32_t chunks);

  virtual int
  SelectDevice();

  virtual void
  SendContent(int deviceID, Name object, uint32_t chunks);

  // Mobility
  virtual void
  Move(Ptr<const MobilityModel> model);

  virtual void
  Session(Ptr<const MobilityModel> model);

  void
  CourseChange(Ptr<const MobilityModel> model);

//  void
//  CourseChangeCallback(string path, Ptr<const MobilityModel> model);

protected:

  virtual void
  StartApplication();

  virtual void
  StopApplication();

private:
  // Interest sending variables
  EventId m_sendEvent;
  Time m_retxTimer;
  EventId m_retxEvent;

  Ptr<RttEstimator> m_rtt;
  Time m_interestLifeTime;

  // Content producing variables
  Name m_prefix;
  Name m_postfix;
  uint32_t m_virtualPayloadSize;
  Time m_freshness;

  uint32_t m_signature;
  Name m_keyLocator;

  // Object requesting data structures
  vector<Name> m_interestQueue;
  list<Name> m_pendingObjects;
  vector<Name> m_retxNames;

  uint32_t m_windowSize;
  uint32_t m_pendingInterests;

  // Content providing data structures
  vector<Name> m_providedObjects;
  vector<Name> m_generatedContent;

  // Shared global variables
  Ptr<NameService> m_nameService;
  Ptr<UniformRandomVariable> m_rand;

  //Pushing strategy data structures
  vector<int> m_vicinity;
  Time m_vicinityTimer;
  uint32_t m_replicationDegree;

  // Structures for retransmission 
  map<Name, Time> m_nameTimeouts;
 
  map<Name, Time> m_nameLastDelay;
  map<Name, Time> m_nameFullDelay;
  map<Name, uint32_t> m_nameRetxCounts;

  TracedCallback<Ptr<App>, Name, Time, int32_t> m_lastRetransmittedInterestDataDelay;
  TracedCallback<Ptr<App>, Name, Time, uint32_t, int32_t> m_firstInterestDataDelay;
 
  SequenceNumber32 m_chunksRetrieved;
  map<Name, SequenceNumber32> m_chunkOrder;

  // Mobility
  bool m_moving;

};

} // namespace ndn
} // namespace ns3

#endif // NDN_MOBILE_USER_H


