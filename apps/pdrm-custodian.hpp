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

#ifndef PDRM_CUSTODIAN_H
#define PDRM_CUSTODIAN_H

#include "pdrm-strategy.hpp"

using namespace std;

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A mobile user that can be a producer, provider, and consumer 
 */
class PDRMCustodian : public PDRMStrategy {
public:
  static TypeId
  GetTypeId();

  // Constructor
  PDRMCustodian();

  // Finalize data structures and print collected statistics
  void
  EndGame();

  virtual void
  OnInterest(shared_ptr<const Interest> interest);

  virtual void
  OnHint(shared_ptr<const Interest> interest);

  void
  ProduceObject();

  virtual void
  ReplicateContent();

  virtual void
  HintContent(Name object);

protected:

  virtual void
  StartApplication();

  virtual void
  StopApplication();

protected:
  Name m_custodianPrefix;
  bool m_custodian;
};

} // namespace ndn
} // namespace ns3

#endif // PDRM_CUSTODIAN_H

