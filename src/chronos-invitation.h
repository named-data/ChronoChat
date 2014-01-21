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


#include <ndn-cpp-dev/name.hpp>
#include <ndn-cpp-dev/signature.hpp>

class ChronosInvitation
{
/*
 * /ndn/broadcast/chronos/invitation/[invitee_namespace]/<chatroom_name>/<inviter_routing_prefix>/<keylocator>/<signature>
 */
  static const size_t NAME_SIZE_MIN;
  static const size_t INVITEE_START;
  static const ssize_t SIGNATURE;
  static const ssize_t KEY_LOCATOR;
  static const ssize_t INVITER_PREFIX;
  static const ssize_t CHATROOM;
  static const ndn::Name INVITATION_PREFIX;

public:
  struct Error : public std::runtime_error { Error(const std::string &what) : std::runtime_error(what) {} };

  ChronosInvitation() {}

  ChronosInvitation(const ndn::Name& interestName);

  ChronosInvitation(const ndn::Name &inviteeNameSpace,
                    const ndn::Name &chatroom,
                    const ndn::Name &inviterRoutingPrefix,
                    const ndn::Name &inviterCertificateName);

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
  getInviterRoutingPrefix() const
  { return m_inviterRoutingPrefix; }

  const ndn::Name&
  getInviterCertificateName() const
  { return m_inviterCertificateName; }

  const ndn::Signature&
  getSignature() const
  { return m_signature; }

  const ndn::Name&
  getInviterNameSpace() const
  { return m_inviterNameSpace; }

  const ndn::Buffer&
  getSignedBlob() const
  { return m_signedBlob; }
  
  const ndn::Name&
  getInterestName() const
  {
    if(m_isSigned)
      return m_interestName; 
    else
      throw Error("Invitation is not signed!");
  }

  void
  setSignatureValue(const ndn::Block &signatureValue);

private:
  ndn::Name m_interestName;
  ndn::Buffer m_signedBlob;

  ndn::Name m_inviteeNameSpace;
  ndn::Name m_chatroom;
  ndn::Name m_inviterRoutingPrefix;
  ndn::Name m_inviterCertificateName;
  ndn::Signature m_signature;

  ndn::Name m_inviterNameSpace;

  bool m_isSigned;
};

#endif
