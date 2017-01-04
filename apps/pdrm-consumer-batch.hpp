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

#ifndef PDRM_CONSUMER_BATCH_H
#define PDRM_CONSUMER_BATCH_H

#include "pdrm-consumer.hpp"

using namespace std;

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A mobile user that can be a producer, provider, and consumer 
 */
class PDRMConsumerBatch : public PDRMConsumer {
public:
  static TypeId
  GetTypeId();

  // Constructor
  PDRMConsumerBatch();
  virtual ~PDRMConsumerBatch(){};

  virtual void
  FindObject();

  void
  ScheduleNextPacket();

  void 
  GetNextPacket();

protected:

  virtual void
  StartApplication();

  virtual void
  StopApplication();

protected:
  uint32_t m_batchSize;
  uint32_t m_requestsSent;
};

} // namespace ndn
} // namespace ns3

#endif // PDRM_CONSUMER_BATCH_H



