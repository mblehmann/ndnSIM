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

  virtual void
  advertiseContent(Name object);

  virtual void
  AnnounceContent();

  virtual void
  AnnounceContent(Name object);

  virtual void
  PushContent();
  
private:
  Name[] m_generatedContent;
  uint32_t m_producedObjects; 

};

} // namespace ndn
} // namespace ns3

#endif // NDN_MOBILE_PRODUCER_H


