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

#ifndef PDRM_STRATEGY_H
#define PDRM_STRATEGY_H

#include "pdrm-mobile-producer.hpp"

using namespace std;

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A mobile user that can be a producer, provider, and consumer 
 */
class PDRMStrategy : public PDRMMobileProducer {
public:
  static TypeId
  GetTypeId();

  // Constructor
  PDRMStrategy();

  // Finalize data structures and print collected statistics
  void
  EndGame();

  virtual void
  PopulateCatalog(uint32_t index);

  virtual void
  UpdateNetwork();

  virtual void
  OnInterest(shared_ptr<const Interest> interest);

  virtual void
  OnHint(shared_ptr<const Interest> interest);

  virtual void
  OnVicinity(shared_ptr<const Interest> interest);

  virtual void
  OnData(shared_ptr<const Data> contentObject);

  virtual void
  OnVicinityData(shared_ptr<const Data> contentObject);

  virtual void
  ScheduleNextPacket();

  virtual void
  ConcludeObjectDownload(Name object);

  void
  ProduceObject();

  virtual void
  ReplicateContent();

  virtual void
  ProbeVicinity(Name object);

  virtual void
  PushContent(Name objectName);

  virtual void
  PushToSelectedDevices(Name objectName);

  virtual void
  PushToRandomDevices(Name objectName);

  virtual PDRMStrategySelectors
  SelectBestDevice(uint32_t location, bool include, double availability);

  virtual void
  HintContent(int deviceID, Name object);

  void
  Session(Ptr<const MobilityModel> model);

  void
  CourseChange(Ptr<const MobilityModel> model);

  typedef void (*ReceivedHintCallBack)(Ptr<App> app, Name object, bool isAccepted);
  typedef void (*ReceivedVicinityDataCallBack)(Ptr<App> app, Name object, int32_t nodeId, int32_t homeNetwork, int32_t preferedLocation, double timeSpent, int32_t currentPosition, double availability, bool isInterested);

  typedef void (*ReplicatedContentCallBack)(Ptr<App> app, Name object);
  typedef void (*ProbedVicinityCallBack)(Ptr<App> app, Name object, int32_t vicinitySize);
  typedef void (*SelectedDeviceCallBack)(Ptr<App> app, Name object, bool isSatisfied, bool isHinted, int nodeId, double producerAvailability);
  typedef void (*HintedContentCallBack)(Ptr<App> app, Name object, int deviceId);

protected:

  virtual void
  StartApplication();

  virtual void
  StopApplication();

protected:
  // Strategy (vicinity, hint, and replication)
  Name m_vicinityPrefix;
  Time m_vicinityTimer;
  uint32_t m_vicinitySize;

  uint32_t m_placementPolicy;

  EventId m_updateNetwork;

  Name m_hintPrefix;
  Time m_hintTimer;
  double m_altruism;

  map<Name, vector<PDRMStrategySelectors> > m_vicinity;
  map<Name, vector<PDRMStrategySelectors> > m_consumers;
  queue<Name> m_pendingReplication;

  TracedCallback<Ptr<App>, Name, bool> m_receivedHint;
  TracedCallback<Ptr<App>, Name, int32_t, int32_t, double, bool> m_receivedVicinityData;
  
  TracedCallback<Ptr<App>, Name> m_replicatedContent;
  TracedCallback<Ptr<App>, Name, int32_t> m_probedVicinity;
  TracedCallback<Ptr<App>, Name, bool, bool, int, double> m_selectedDevice;
  TracedCallback<Ptr<App>, Name, int> m_hintedContent;
};

} // namespace ndn
} // namespace ns3

#endif // PDRM_STRATEGY_H

