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

#include "ndn-mobile-producer.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"

NS_LOG_COMPONENT_DEFINE("ndn.MobileProducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(MobileProducer);

TypeId
MobileProducer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::MobileProducer")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<MobileProducer>()

  return tid;
}

MobileProducer::MobileProducer()
{
}

/**
 * Create a new content.
 * Sets it popularity through advertisement.
 */
void
MobileProducer::PublishContent()
{
  newObject = m_prefix + m_postfix + m_producedObjects;
  m_generatedContent.add(newObject);

  advertiseContent(newObject);

  AnnounceContent(newObject);

  PushContent(newObject);

}

/**
 * Advertise a new content.
 * Puts it into a shared data structure and set who should request it (popularity of the content).
 */
void
MobileProducer::advertiseContent(Name newObject)
{
  allContents.add(newObject);
  setContentPopularity();
}

/**
 *
 */
void
MobileProducer::AnnounceContent()
{
  for (Name object : m_generatedContent) {
    AnnounceContent(object);
  }
}

/**
 *
 */
void
MobileProducer::AnnounceContent(Name object)
{
  auto announcement = make_shared<Announcement>();
  announcement->setName(object);

  announcement->wireEncode();

  m_transmittedDatas(announcement, this, m_face);
  m_face->onReceiveAnnouncement(*announcement);
}

/**
 *
 */
void
MobileProducer::PushContent()
{
  discoverVicinity();

  selectDevice();

  sendContent();
}


} // namespace ndn
} // namespace ns3


