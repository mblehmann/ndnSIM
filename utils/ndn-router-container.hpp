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
 **/

#ifndef NDN_ROUTER_CONTAINER_H
#define NDN_ROUTER_CONTAINER_H

//#include "ns3/ndnSIM/model/ndn-common.hpp"

#include <stdint.h>
#include <vector>

#include "ns3/ptr.h"
#include "ns3/simple-ref-count.h"
#include "ndn-mobility-profile.hpp"

using namespace std;

namespace ns3 {
namespace ndn {

class RouterContainer : public SimpleRefCount<Node> {
private:
  typedef vector<Ptr<Node>> Container;

public:
  typedef Container::const_iterator Iterator; ///< @brief Iterator over MobilityProfileContainer

  /**
   * @brief Create an empty MobilityProfileContainer.
   */
  RouterContainer();

  /**
   * @brief Copy constructor for MobilityProfileContainer. Calls AddAll method
   *
   * @see MobilityProfileContainer::AddAll
   */
  RouterContainer(const RouterContainer& other);

  /**
   * @brief Copy operator for MobilityProfileContainer. Empties vector and calls AddAll method
   *
   * All previously obtained iterators (Begin() and End()) will be invalidated
   *
   * @see MobilityProfileContainer::AddAll
   */
  RouterContainer&
  operator=(const RouterContainer& other);

  /**
   * Add an entry to the container
   *
   * @param face a smart pointer to a MobilityProfile-derived object
   */
  void
  Add(Ptr<Node> router);

  /**
   * @brief Add all entries from other container
   *
   * @param other smart pointer to a container
   */
  void
  AddAll(Ptr<RouterContainer> other);

  /**
   * @brief Add all entries from other container
   *
   * @param other container
   */
  void
  AddAll(const RouterContainer& other);

public: // accessors
  /**
   * @brief Get an iterator which refers to the first pair in the
   * container.
   *
   * @returns an iterator which refers to the first pair in the container.
   */
  Iterator
  Begin() const;

  /**
   * @brief Get an iterator which indicates past-the-last Node in the
   * container.
   *
   * @returns an iterator which indicates an ending condition for a loop.
   */
  Iterator
  End() const;

  /**
   * @brief Get the number of profiles stored in this container
   *
   * @returns the number of profiles stored in this container
   */
  uint32_t
  GetN() const;

  /**
   * Get a MobilityProfile stored in the container
   *
   * @param pos index of the MobilityProfile in the container
   * @throw std::out_of_range if !(pos < GetN()).
   */
  Ptr<Node>
  Get(size_t pos) const;

private:
  Container m_routers;
};

} //ndn
} //ns3

#endif 
