/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "invitation.hpp"

#include <ndn-cxx/security/signature-sha256-with-rsa.hpp>

#include "logging.h"

namespace chronochat {

using std::string;

using ndn::IdentityCertificate;

const size_t  Invitation::NAME_SIZE_MIN         = 7;
const ssize_t Invitation::SIGNATURE             = -1;
const ssize_t Invitation::KEY_LOCATOR           = -2;
const ssize_t Invitation::TIMESTAMP             = -3;
const ssize_t Invitation::INVITER_CERT          = -4;
const ssize_t Invitation::INVITER_PREFIX        = -5;
const ssize_t Invitation::CHATROOM              = -6;
const ssize_t Invitation::CHRONOCHAT_INVITATION = -7;


Invitation::Invitation(const Name& interestName)
{
  size_t nameSize = interestName.size();

  if (nameSize < NAME_SIZE_MIN)
    throw Error("Wrong Invitation Name: Wrong length");

  if (interestName.get(CHRONOCHAT_INVITATION).toUri() != "CHRONOCHAT-INVITATION")
    throw Error("Wrong Invitation Name: Wrong application tags");

  m_interestName = interestName.getPrefix(KEY_LOCATOR);
  m_timestamp = interestName.get(TIMESTAMP).toNumber();
  m_inviterCertificate.wireDecode(interestName.get(INVITER_CERT).blockFromValue());
  m_inviterRoutingPrefix.wireDecode(interestName.get(INVITER_PREFIX).blockFromValue());
  m_chatroom = interestName.get(CHATROOM).toUri();
  m_inviteeNameSpace = interestName.getPrefix(CHRONOCHAT_INVITATION);
}

Invitation::Invitation(const Name& inviteeNameSpace,
                       const string& chatroom,
                       const Name& inviterRoutingPrefix,
                       const IdentityCertificate& inviterCertificate)
  : m_inviteeNameSpace(inviteeNameSpace)
  , m_chatroom(chatroom)
  , m_inviterRoutingPrefix(inviterRoutingPrefix)
  , m_inviterCertificate(inviterCertificate)
  , m_timestamp(time::toUnixTimestamp(time::system_clock::now()).count())
{
  m_interestName = m_inviteeNameSpace;
  m_interestName.append("CHRONOCHAT-INVITATION")
    .append(m_chatroom)
    .append(m_inviterRoutingPrefix.wireEncode())
    .append(m_inviterCertificate.wireEncode())
    .append(name::Component::fromNumber(m_timestamp));
}

Invitation::Invitation(const Invitation& invitation)
  : m_interestName(invitation.m_interestName)
  , m_inviteeNameSpace(invitation.m_inviteeNameSpace)
  , m_chatroom(invitation.m_chatroom)
  , m_inviterRoutingPrefix(invitation.m_inviterRoutingPrefix)
  , m_inviterCertificate(invitation.m_inviterCertificate)
  , m_timestamp(invitation.m_timestamp)
{
}

} // namespace chronochat
