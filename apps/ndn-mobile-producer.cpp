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
#include "ndn-mobile-user.hpp"
#include "ndn-producer.hpp"
#include "model/ndn-app-face.hpp"

#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/pointer.h"

#include "helper/ndn-fib-helper.hpp"

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
      .SetParent<Producer>()
      .AddConstructor<MobileProducer>()

      .AddAttribute("ReplicationDegree", "Number of replicas pushed",
                   UintegerValue(1), MakeUintegerAccessor(&MobileProducer::m_replicationDegree), MakeUintegerChecker<uint32_t>())

      .AddAttribute("VicinityTimer", "Period to discover vicinity",
                   StringValue("10s"), MakeTimeAccessor(&MobileProducer::m_vicinityTimer), MakeTimeChecker())

      .AddAttribute("NameService", "Name service to discover content",
                   PointerValue(NULL),
                   MakePointerAccessor(&MobileProducer::m_nameService),
                   MakePointerChecker<NameService>());

  return tid;
}

MobileProducer::MobileProducer()
  : m_rand(CreateObject<UniformRandomVariable>())
{
  NS_LOG_FUNCTION_NOARGS();
}

// Application Methods
void
MobileProducer::StartApplication() // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS();
  Producer::StartApplication();

//  App::StartApplication();
//  FibHelper::AddRoute(GetNode(), m_prefix, m_face, 0);

  NS_LOG_INFO("> Starting the producer");

  Simulator::Schedule(Time("1s"), &MobileProducer::PublishContent, this);
}

/*
void
MobileProducer::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // tracing inside

  NS_LOG_FUNCTION(this << interest);
}
*/
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
  NS_LOG_INFO("> Publishing a new object");

  Name newObject = createContent();

  // Advertise the new content
  advertiseContent(newObject);

  AnnounceContent(newObject);

  discoverVicinity(newObject);

  m_pushedObjects.push(newObject);

  // scheduling events!!
  Simulator::Schedule(m_vicinityTimer, &MobileProducer::PushContent, this);

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
  //newObject->append(m_postfix);
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
  uint32_t chunks = 7;

  vector<Ptr<Node>> users = m_nameService->getUsers();
  Ptr<Node> currentUser;
  Ptr<MobileUser> mobileUser;
  for (uint32_t i = 0; i < users.size(); i++) {
    Ptr<Node> currentUser = users[i];
    Ptr<MobileUser> mobileUser = DynamicCast<MobileUser> (currentUser->GetApplication(0));
    //Simulator::ScheduleNow(&MobileUser::AddInterestObject, &newObject, 3);
    //Simulator::ScheduleNow(&MobileUser::AddInterestObject, &users[i], &newObject, 3);
    //Simulator::ScheduleNow(&MobileUser::AddInterestObject, dynamic_cast<MobileUser*>(&users[i]->GetApplication(0)), &newObject, 3);
    Simulator::ScheduleWithContext(currentUser->GetId(), Time("0s"), &MobileUser::AddInterestObject, mobileUser, newObject, chunks);
  }

  NS_LOG_INFO("> Advertising object " << newObject);
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
  NS_LOG_INFO("> Creating the announcement for object " << object);
  
  NS_LOG_INFO("> Generating the Nonce " << m_rand->GetValue(0, numeric_limits<uint32_t>::max()));
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
  shared_ptr<Vicinity> vicinity = make_shared<Vicinity>(object);
  NS_LOG_INFO("> Discovering the vicinity for object " << object);

  // Log information
  NS_LOG_INFO("> Vicinity discovery for " << object);

  // Send the packet
  vicinity->wireEncode();

  m_transmittedVicinities(vicinity, this, m_face);
  m_face->onReceiveVicinity(*vicinity);
}

void
MobileProducer::OnVicinityData(shared_ptr<const VicinityData> vicinityData)
{
  Name objectName = vicinityData->getName();
  int remoteNode = vicinityData->getNodeID();
  NS_LOG_INFO("> Vicinity data response RECEIVED from " << remoteNode << " for " << objectName);

  m_vicinity.push_back(remoteNode);
}

/**
 * Execute the whole push content operation
 */
void
MobileProducer::PushContent()
{
  Name objectName = m_pushedObjects.front();
  m_pushedObjects.pop();

  NS_LOG_INFO("> Pushing the object " << objectName);

  if (m_vicinity.size() > 0) {

    NS_LOG_INFO("> We have neighbours " << m_vicinity.size());
    int deviceID;

    for (uint32_t i = 0; i < m_replicationDegree; i++) {
      deviceID = selectDevice();
      NS_LOG_INFO("> Selected device " << deviceID);
      sendContent(deviceID, objectName);
    }
  }
}

int
MobileProducer::selectDevice()
{
  int position = m_rand->GetValue(0, m_vicinity.size());
  return m_vicinity[position];
}

void
MobileProducer::sendContent(int deviceID, Name objectName)
{
  NS_LOG_INFO("> Sending " << objectName << " to " << deviceID);
  // Create the vicinity packet
  shared_ptr<Hint> hint = make_shared<Hint>(objectName);
  hint->setNodeID(deviceID);
  hint->setScope(2);

  // Log information
  NS_LOG_INFO("> Hint to " << deviceID << " for " << objectName);

  // Send the packet
  hint->wireEncode();

  m_transmittedHints(hint, this, m_face);
  m_face->onReceiveHint(*hint);
}

void
MobileProducer::OnAnnouncement(shared_ptr<const Announcement> announcement)
{
  App::OnAnnouncement(announcement);

  NS_LOG_INFO("> LE ANNOUNCEMENT");
}


} // namespace ndn
} // namespace ns3


