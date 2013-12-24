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

  const ndn::Blob&
  getSignatureBits() const
  { return m_signatureBits; }

  const ndn::Name&
  getInviterNameSpace() const
  { return m_inviterNameSpace; }

  const ndn::Blob&
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
  ndn::Blob m_signatureBits;
  ndn::Name m_inviterNameSpace;

  ndn::Blob m_signedBlob;
};

#endif
