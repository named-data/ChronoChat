/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOS_INVITATION_H
#define CHRONOS_INVITATION_H


#include <ndn-cpp/name.hpp>

class ChronosInvitation
{
public:
  struct Error : public std::runtime_error { Error(const std::string &what) : std::runtime_error(what) {} };

  ChronosInvitation() {}

  ChronosInvitation(const ndn::Name& interestName);

  ChronosInvitation(const ChronosInvitation& invitation);

  virtual
  ~ChronosInvitation() {};

  const ndn::Name&
  getInviteeNameSpace() const
  { return m_inviteeNameSpace; }

  const ndn::Name&
  getChatroom() const
  { return m_chatroom; }

  const ndn::Name&
  getInviterPrefix() const
  { return m_inviterPrefix; }

  const ndn::Name&
  getInviterCertificateName() const
  { return m_inviterCertificateName; }

  const ndn::Buffer&
  getSignatureBits() const
  { return m_signatureBits; }

  const ndn::Name&
  getInviterNameSpace() const
  { return m_inviterNameSpace; }

  const ndn::Buffer&
  getSignedBlob() const
  { return m_signedBlob; }
  
  const ndn::Name&
  getInterestName() const
  { return m_interestName; }

private:
  ndn::Name m_interestName;

  ndn::Name m_inviteeNameSpace;
  ndn::Name m_chatroom;
  ndn::Name m_inviterPrefix;
  ndn::Name m_inviterCertificateName;
  ndn::Buffer m_signatureBits;
  ndn::Name m_inviterNameSpace;

  ndn::Buffer m_signedBlob;
};

#endif
