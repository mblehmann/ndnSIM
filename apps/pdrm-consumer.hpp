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

#ifndef PDRM_CONSUMER_H
#define PDRM_CONSUMER_H

#include "ndn-app.hpp"

#include "ns3/ndnSIM/utils/ndn-rtt-estimator.hpp"

#include <utils/pdrm-catalog.hpp>
#include <utils/pdrm-global.hpp>

using namespace std;

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A mobile user that can be a producer, provider, and consumer 
 */
class PDRMConsumer : public App {
public:
  static TypeId
  GetTypeId();

  // Constructor
  PDRMConsumer();
  virtual ~PDRMConsumer(){};

  // Finalize data structures and print collected statistics
  void
  EndGame();

  void
  WarmUp();

  virtual void
  OnTimeout(Name chunk);

  virtual void
  OnData(shared_ptr<const Data> contentObject);

  virtual void
  FindObject();

  virtual void
  FoundObject(Name object);

  virtual void
  StartObjectDownload(ContentObject object);

  virtual void
  ScheduleNextPacket();

  void 
  GetNextPacket();

  void 
  SendPacket(Name chunk, bool retransmission);

  virtual void
  WillSendOutInterest(Name chunk);

  virtual void
  ConcludeObjectDownload(Name object);

  typedef void (*ChunkRetrievalDelayCallback)(Ptr<App> app, Name chunk, uint32_t order, Time totalDelay, Time lastDelay, uint32_t requestCount, uint32_t hopCount);
  typedef void (*ObjectDownloadTimeCallback)(Ptr<App> app, Name object, Time download, uint32_t requests);
  typedef void (*ChunkFailedDelayCallback)(Ptr<App> app, Name chunk, uint32_t order, Time totalDelay, Time lastDelay, uint32_t requestCount, uint32_t hopCount);
  typedef void (*ObjectFailedDownloadCallback)(Ptr<App> app, Name object, Time download, uint32_t requests);

protected:

  virtual void
  StartApplication();

  virtual void
  StopApplication();

protected:
  Time m_warmupPeriod;
  Time m_start;
  Time m_end;
  bool m_warmup;
  bool m_execution;

  // Sending Interest Packets
  EventId m_sendEvent;               // EventId of pending "send packet" event
  Ptr<UniformRandomVariable> m_rand; // nonce generator
  Time m_interestLifeTime;           // lifeTime for interest packet

  queue<Name> m_chunkRequest;        // queue of chunks to request

  // retransmission data structures
  map<Name, EventId> m_retxEvent;      // retransmission event per chunk

  // Content catalog (global structure)
  Ptr<PDRMCatalog> m_catalog;
  Ptr<PDRMGlobal> m_global;

  // consumer characteristic
  double m_lambdaRequests;
  Ptr<ConstantRandomVariable> m_requestPeriod;
  bool m_defaultConsumer;
  bool m_localConsumer; // locality
  uint32_t m_position;

  // chunk tracing data structures
  map<Name, Time> m_chunkFirstRequest;
  map<Name, Time> m_chunkLastRequest;
  map<Name, uint32_t> m_chunkRequestCount;
  uint32_t m_maxHopCount;

  // object tracing data structures
  map<Name, Time> m_objectStartDownloadTime;
  map<Name, uint32_t> m_objectSize;
  map<Name, uint32_t> m_objectRequests;
  map<Name, uint32_t> m_objectTimeouts;
  map<Name, map<uint32_t, bool> > m_chunksMap;
  map<Name, uint32_t> m_chunksDownloaded;

  // Tracers
  // Application, chunk name, chunk order, total delay, last request delay, request count, hop count
  // if retx count == 1; total delay = last delay
  TracedCallback<Ptr<App>, Name, uint32_t, Time, Time, uint32_t, uint32_t> m_chunkRetrievalDelay;
  
  // Application, object name, download time, total requests
  TracedCallback<Ptr<App>, Name, Time, uint32_t> m_objectDownloadTime;
 
  TracedCallback<Ptr<App>, Name, uint32_t, Time, Time, uint32_t, uint32_t> m_chunkFailedDelay;
  TracedCallback<Ptr<App>, Name, Time, uint32_t> m_objectFailedDownload;
};

} // namespace ndn
} // namespace ns3

#endif // PDRM_CONSUMER_H



