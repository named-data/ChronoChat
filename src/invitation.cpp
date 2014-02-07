/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "invitation.h"

#include <ndn-cpp-dev/security/identity-certificate.hpp>
#include <ndn-cpp-dev/security/signature-sha256-with-rsa.hpp>

#include "logging.h"

using namespace std;
using namespace ndn;

INIT_LOGGER("Invitation");


namespace chronos{

const size_t  Invitation::NAME_SIZE_MIN  = 9;
const size_t  Invitation::INVITEE_START  = 4;
const ssize_t Invitation::SIGNATURE      = -1;
const ssize_t Invitation::KEY_LOCATOR    = -2;
const ssize_t Invitation::TIMESTAMP      = -3;
const ssize_t Invitation::INVITER_PREFIX = -4;
const ssize_t Invitation::CHATROOM       = -5;
const Name    Invitation::INVITATION_PREFIX("/ndn/broadcast/chronos/chat-invitation");


Invitation::Invitation(const Name& interestName)
{
  size_t nameSize = interestName.size();

  if(nameSize < NAME_SIZE_MIN)
    throw Error("Wrong Invitation Name: Wrong length"); 
 
  if(!INVITATION_PREFIX.isPrefixOf(interestName))
    throw Error("Wrong Invitation Name: Wrong invitation prefix");

  m_interestName = interestName.getPrefix(-3);
  m_inviterRoutingPrefix.wireDecode(interestName.get(INVITER_PREFIX).blockFromValue());
  m_chatroom.wireDecode(interestName.get(CHATROOM).blockFromValue());
  m_inviteeNameSpace = interestName.getSubName(INVITEE_START, nameSize - NAME_SIZE_MIN);  
}

Invitation::Invitation(const Name &inviteeNameSpace,
                       const Name &chatroom,
                       const Name &inviterRoutingPrefix)
  : m_inviteeNameSpace(inviteeNameSpace)
  , m_chatroom(chatroom)
  , m_inviterRoutingPrefix(inviterRoutingPrefix)
{  
  m_interestName = INVITATION_PREFIX;
  m_interestName.append(inviteeNameSpace).append(chatroom.wireEncode()).append(inviterRoutingPrefix.wireEncode());
}

Invitation::Invitation(const Invitation& invitation)
  : m_interestName(invitation.m_interestName)
  , m_inviteeNameSpace(invitation.m_inviteeNameSpace)
  , m_chatroom(invitation.m_chatroom)
  , m_inviterRoutingPrefix(invitation.m_inviterRoutingPrefix)
{}

}//chronos
