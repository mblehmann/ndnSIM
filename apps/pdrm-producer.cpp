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

#include "pdrm-producer.hpp"

#include "helper/ndn-fib-helper.hpp"
#include "helper/ndn-global-routing-helper.hpp"

using namespace std;

NS_LOG_COMPONENT_DEFINE("ndn.PDRMProducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(PDRMProducer);

TypeId
PDRMProducer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::PDRMProducer")
      .SetGroupName("Ndn")
      .SetParent<PDRMProvider>()
      .AddConstructor<PDRMProducer>()

      // Producer
      .AddAttribute("ProducingInterval",
                    "Interval between publication of new objects",
                    StringValue("0s"),
                    MakeTimeAccessor(&PDRMProducer::m_producingInterval),
                    MakeTimeChecker())

      .AddAttribute("ProducerPrefix",
                    "Prefix, for which producer has the data",
                    StringValue("/producer"),
                    MakeNameAccessor(&PDRMProducer::m_producerPrefix),
                    MakeNameChecker())

      .AddAttribute("ObjectPrefix",
                    "Object prefix for this producer",
                    StringValue("/object"),
                    MakeNameAccessor(&PDRMProducer::m_objectPrefix),
                    MakeNameChecker())

      .AddTraceSource("ProducedObject",
                      "Objects produced by the producer",
                      MakeTraceSourceAccessor(&PDRMProducer::m_producedObject),
                      "ns3::ndn::PDRMProducer::ProducedObjectCallback");

  return tid;
}

PDRMProducer::PDRMProducer()
{
}

/**
 * 
 */
void
PDRMProducer::StartApplication()
{
  PDRMProvider::StartApplication();

  // update the producer prefix with the node ID
  m_producerPrefix = m_producerPrefix.toUri() + to_string(GetNode()->GetId());
  AnnouncePrefix(m_producerPrefix);

  if (!m_producingInterval.IsZero()) {
    Simulator::Schedule(m_producingInterval, &PDRMProducer::ProduceObject, this);
    Simulator::Schedule(Time("100ms"), &PDRMProducer::WarmUp, this);
  }
}


/**
 * 
 */
void
PDRMProducer::StopApplication()
{
  PDRMProvider::StopApplication();
}

void
PDRMProducer::EndGame()
{
}

void
PDRMProducer::WarmUp()
{
  NS_LOG_FUNCTION_NOARGS();
  uint32_t catalogSize = m_catalog->getCatalogSize();

  vector<int> index_list;
  for (uint32_t index = 0; index < catalogSize; index++) {
    index_list.push_back(index);
  }
  random_shuffle(index_list.begin(), index_list.end());

  uint32_t index;
  while (index_list.size() > 0) {
    index = index_list.back();
    PopulateCatalog(index);
    index_list.pop_back();
  }

  m_warmup = false;
  m_execution = true;
}

/**
 *
 */
void
PDRMProducer::PopulateCatalog(uint32_t index)
{
  //get content name
  int objectIndex = m_producedObjects.size();

  shared_ptr<Name> object = make_shared<Name>(m_producerPrefix);
  Name objectName = m_objectPrefix.toUri() + to_string(objectIndex);
  object->append(objectName);

  m_producedObjects.insert(*object);
  m_lastProducedObject = *object;

  //generate content
  m_catalog->addObject(*object, index);

  ContentObject co = m_catalog->getObject(*object);
  uint32_t popularity = m_catalog->getObjectPopularity(*object);
  m_producedObject(this, *object, co.size, co.availability, popularity);
}

/**
 *
 */
void
PDRMProducer::ProduceObject()
{
  NS_LOG_INFO(Simulator::Now() << " " << m_end << " " << m_execution);
  if (Simulator::Now() > m_end) {
    m_execution = false;
    return;
  }

  NS_LOG_FUNCTION_NOARGS();
  //get content name
  int objectIndex = m_producedObjects.size();

  shared_ptr<Name> object = make_shared<Name>(m_producerPrefix);
  Name objectName = m_objectPrefix.toUri() + to_string(objectIndex);
  object->append(objectName);

  m_producedObjects.insert(*object);
  m_lastProducedObject = *object;

  //generate content
  m_catalog->addObject(*object);

  ContentObject co = m_catalog->getObject(*object);
  uint32_t popularity = m_catalog->getObjectPopularity(*object);
  m_producedObject(this, *object, co.size, co.availability, popularity);
  NS_LOG_FUNCTION_NOARGS();

  Simulator::Schedule(m_producingInterval, &PDRMProducer::ProduceObject, this);
}

} // namespace ndn
} // namespace ns3


