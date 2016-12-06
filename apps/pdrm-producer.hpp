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

#ifndef PDRM_PRODUCER_H
#define PDRM_PRODUCER_H

#include "pdrm-provider.hpp"

using namespace std;

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A mobile user that can be a producer, provider, and consumer 
 */
class PDRMProducer : public PDRMProvider {
public:
  static TypeId
  GetTypeId();

  // Constructor
  PDRMProducer();

  // Finalize data structures and print collected statistics
  void
  EndGame();

  void
  WarmUp();

  virtual void
  PopulateCatalog(uint32_t index);

  /**
   * Producer actions
   * A producer publishes content and advertise it
   */
  virtual void
  ProduceObject();

  typedef void (*ProducedObjectCallback)(Ptr<App> app, Name object, uint32_t size, double availability, uint32_t popularity);

protected:

  virtual void
  StartApplication();

  virtual void
  StopApplication();

protected:

  // Produced content
  Time m_producingInterval;
  Name m_producerPrefix;
  Name m_objectPrefix;

  set<Name> m_producedObjects;
  Name m_lastProducedObject;

  TracedCallback<Ptr<App>, Name, uint32_t, double, uint32_t> m_producedObject; 
};

} // namespace ndn
} // namespace ns3

#endif // PDRM_PRODUCER_H



