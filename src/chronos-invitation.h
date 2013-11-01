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


#include <ndn.cxx/fields/name.h>

class ChronosInvitation
{
public:
  ChronosInvitation() {}

  ChronosInvitation(const ndn::Name& interestName);

  ChronosInvitation(const ChronosInvitation& invitation);

  virtual
  ~ChronosInvitation() {};

  inline const ndn::Name&
  getInviteeNameSpace() const
  { return m_inviteeNameSpace; }

  inline const ndn::Name&
  getChatroom() const
  { return m_chatroom; }

  inline const ndn::Name&
  getInviterPrefix() const
  { return m_inviterPrefix; }

  inline const ndn::Name&
  getInviterCertificateName() const
  { return m_inviterCertificateName; }

  inline const ndn::Blob&
  getSignatureBits() const
  { return m_signatureBits; }

  inline const ndn::Name&
  getInviterNameSpace() const
  { return m_inviterNameSpace; }

  inline const ndn::Blob&
  getSignedBlob() const
  { return m_signedBlob; }
  
  inline const ndn::Name&
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
