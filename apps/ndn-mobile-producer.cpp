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

#include <ndn-cxx/announcement.hpp>
#include <ndn-cxx/hint.hpp>
#include <ndn-cxx/vicinity.hpp>
#include <ndn-cxx/vicinity-data.hpp>

using namespace std;

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
      .AddConstructor<MobileProducer>();

  return tid;
}

MobileProducer::MobileProducer()
{
}

void
MobileProducer::setVicinityTimer(Time vicinityTimer)
{
  m_vicinityTimer = vicinityTimer;
}

void
MobileProducer::setReplicationDegree(uint32_t replicationDegree)
{
  m_replicationDegree = replicationDegree;
}



/**
 * Create a new content.
 * Sets it popularity through advertisement.
 */
void
MobileProducer::PublishContent()
{
  Name newObject = createContent();

  // Advertise the new content
  advertiseContent(newObject);

  AnnounceContent(newObject);

  discoverVicinity(newObject);

  // scheduling events!!
  Simulator::Schedule(m_vicinityTimer, &MobileProducer::PushContent, this, &newObject);
}

/**
 * Create a new content.
 */
Name
MobileProducer::createContent()
{
  // Get the index of the next object
  int objectIndex = m_generatedContent.size();

  // Name is /producer/object<index>
  // Objects have segments /producer/object<index>/#seg
  shared_ptr<Name> newObject = make_shared<Name>(m_prefix);
  newObject->append(m_postfix);
  newObject->append(to_string(objectIndex));

  // Add to the list of generated content
  m_generatedContent.push_back(*newObject);

  return *newObject;
}

/**
 * Advertise a new content.
 * Puts it into a shared data structure and set who should request it (popularity of the content).
 * I don't know yet how to do it.
 */
void
MobileProducer::advertiseContent(Name newObject)
{
  m_nameService->publishContent(newObject);
  m_nameService->setRequests(newObject);
}

/**
 * Announce all the objects
 */
void
MobileProducer::AnnounceContent()
{
  uint32_t publishedObjects = m_generatedContent.size();
  for (uint32_t i = 0; i < publishedObjects; i++) {
    AnnounceContent(m_generatedContent[i]);
  }
}

/**
 * Announce a specific object
 */
void
MobileProducer::AnnounceContent(Name object)
{
  // Create the announcement packet
  shared_ptr<Announcement> announcement = make_shared<Announcement>(object);
  announcement->setNonce(m_rand->GetValue(0, numeric_limits<uint32_t>::max()));

  // Log information
  NS_LOG_INFO("> Announcement for " << object);

  // Send the packet
  announcement->wireEncode();

  m_transmittedAnnouncements(announcement, this, m_face);
  m_face->onReceiveAnnouncement(*announcement);
}

/**
 * Send packets to discover the vicinity
 */
void
MobileProducer::discoverVicinity(Name object)
{
  m_vicinity.clear();

  // Create the vicinity packet
//  shared_ptr<Vicinity> vicinity = make_shared<Vicinity>();
//  vicinity->setName(object);

  // Log information
  NS_LOG_INFO("> Vicinity discovery for " << object);

  // Send the packet
//  vicinity->wireEncode();

//  m_transmittedVicinities(vicinity, this, m_face);
//  m_face->onReceiveVicinity(*vicinity);
}

void
MobileProducer::OnVicinityData(shared_ptr<const VicinityData> vicinityData)
{
  m_vicinity.push_back(vicinityData->getName());
}

/**
 * Execute the whole push content operation
 */
void
MobileProducer::PushContent(Name* object)
{
  Name device;

  for (uint32_t i = 0; i < m_replicationDegree; i++) {
    device = selectDevice();
    sendContent(device, *object);
  }
}

Name
MobileProducer::selectDevice()
{
  int position = m_rand->GetValue(0, m_vicinity.size());
  return m_vicinity[position];
}

void
MobileProducer::sendContent(Name device, Name object)
{
  // Create the vicinity packet
//  shared_ptr<Hint> hint = make_shared<Hint>();
//  hint.setName(object);

  // Log information
  NS_LOG_INFO("> Hint to " << device << " for " << object);

  // Send the packet
//  hint.wireEncode();

//  m_transmittedHints(hint, this, m_face);
//  m_face->onReceiveHint(*hint);
}


} // namespace ndn
} // namespace ns3


