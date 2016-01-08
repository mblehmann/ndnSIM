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

#ifndef NDN_MOBILE_PRODUCER_H
#define NDN_MOBILE_PRODUCER_H
#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-producer.hpp"
#include "ns3/traced-value.h"

#include <list>

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A mobile producer that generates content, announce it, push it, and provides it.
 */
class MobileProducer : public Producer {
public:
  static TypeId
  GetTypeId();

  /* Default Constructor */
  MobileProducer();

  virtual void
  PublishContent();

  virtual Name
  createContent();
  
  virtual void
  advertiseContent(Name object);

  virtual void
  AnnounceContent();

  virtual void
  AnnounceContent(Name object);

  virtual void
  PushContent();

  virtual void
  discoverVicinity();

  virtual void
  OnVicinityData(shared_ptr<const VicinityData> vicinityData);

public:
  typedef void (*AnnouncementTraceCallback)(shared_ptr<const Announcement>, Ptr<App>, shared_ptr<Face>);
  typedef void (*VicinityTraceCallback)(shared_ptr<const Vicinity>, Ptr<App>, shared_ptr<Face>);
  typedef void (*HintTraceCallback)(shared_ptr<const Vicinity>, Ptr<App>, shared_ptr<Face>);
  
private:
  std::list<Name> m_generatedContent;

  std::list<Name> m_vicinity;

  uint32_t m_vicinityTimer;
  uint32_t m_replicationDegree;

  Ptr<UniformRandomVariable> m_rand; ///< @brief nonce generator

  TracedCallback<shared_ptr<const Announcement>, Ptr<App>, shared_ptr<Face>>
    m_transmittedAnnouncements; ///< @brief App-level trace of transmitted Announcement

  TracedCallback<shared_ptr<const Vicinity>, Ptr<App>, shared_ptr<Face>>
    m_transmittedVicinities; ///< @brief App-level trace of transmitted Vicinity

  TracedCallback<shared_ptr<const Hint>, Ptr<App>, shared_ptr<Face>>
    m_transmittedHints; ///< @brief App-level trace of transmitted Hint

};

} // namespace ndn
} // namespace ns3

#endif // NDN_MOBILE_PRODUCER_H


