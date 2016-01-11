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

#include "ndn-mobile-user.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
 
#include <list>

using namespace std;

NS_LOG_COMPONENT_DEFINE("ndn.MobileUser");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(MobileUser);

TypeId
MobileUser::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::MobileUser")
      .SetGroupName("Ndn")
      .SetParent<App>()
      .AddConstructor<MobileUser>()
      .AddAttribute("InterestQueue", "Queue of next interest packets to send", StringValue(),
                    MakeNameAccessor(&MobileUser::m_interestQueue), MakeNameChecker())
      .AddAttribute("WindowsSize", "Interest windows size", IntegerValue(0),
                    MakeIntegerAccessor(&MobileUser::m_windowsSize), MakeIntegerChecker<int32_t>());

  return tid;
}

MobileUser::MobileUser()
  : m_windowsSize(1)
{
}

/**
 * Receives the data.
 * Removes the interest packet on the queue.
 * Checks if it is the last chunk of the object (for measurement purpose).
 * Schedules next packet to request.
 */
void
MobileUser::OnData(shared_ptr<const Data> data)
{
  // // Do the regular processing
  // Consumer::OnData(data);
  // objectName = data->getName()

  // // Reduce the number of pending interest requests
  // m_windowsSize--;

  // // Remove the interest from the packet queue
  // m_interestQueue.remove(objectName);

  // // Check if it is the last chunk
  // ConcludeObjectDownload(objectName)

  // // Schedules next packets
  // ScheduleNextPacket();
}

/**
 * 
 */
void
MobileUser::OnTimeout(Name objectName)
{

}

/**
 * 
 */
void
MobileUser::SendPacket()
{

}

/**
 * 
 */
void
MobileUser::WillSendOutInterest(Name objectName)
{

}

/**
 * 
 */
void
MobileUser::ScheduleNextPacket()
{
}

/**
 * Adds a new object to the interest queue. It creates the name of the interest requests and puts it into the interest queue.
 */
void
MobileUser::AddInterestObject(Name objectName, uint32_t chunks)
{
  //create the interest name
  shared_ptr<Name> interestName = make_shared<Name>(objectName);

  //for each chunk, append the sequence number and add it to the interest queue
  for (uint32_t i = 0; i < chunks; i++) {
    interestName->appendSequenceNumber(i);
    m_interestQueue.push_back(interestName);
  }
}

/**
 * Get performance measurements of the download
 */
void
MobileUser::ConcludeObjectDownload(Name objectName)
{
  // if (m_interestQueue.isLastChunk(objectName)) {
  //   NS_LOG_INFO("Finished Dowlonading:" << objectName);
  // }
}


} // namespace ndn
} // namespace ns3


