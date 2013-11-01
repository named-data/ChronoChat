/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "chronos-invitation.h"
#include "exception.h"
#include "logging.h"

using namespace std;
using namespace ndn;

INIT_LOGGER("ChronosInvitation");

ChronosInvitation::ChronosInvitation(const ndn::Name& interestName)
  : m_interestName(interestName)
{
  if(interestName.get(0).toUri() != string("ndn")
     || interestName.get(1).toUri() != string("broadcast")
     || interestName.get(2).toUri() != string("chronos")
     || interestName.get(3).toUri() != string("invitation"))
    throw LnException("Wrong ChronosInvitation Name");
    
  int i = 4;
  int size = interestName.size();

  string chatroomStr("chatroom");
  int inviteeBegin = 4;
  for(; i < size; i++)
    if(interestName.get(i).toUri() == chatroomStr)
      break;

  if(i >= size)
    throw LnException("Wrong ChronosInvitation Name, No chatroom tag");
  m_inviteeNameSpace = interestName.getSubName(inviteeBegin, i - inviteeBegin);

  _LOG_DEBUG(m_inviteeNameSpace.toUri());

  string inviterPrefixStr("inviter-prefix");
  int chatroomBegin = (++i);
  for(; i < size;  i++)
    if(interestName.get(i).toUri() == inviterPrefixStr)
      break;

  if(i > size)
    throw LnException("Wrong ChronosInvitation Name, No inviter-prefix tag");
  m_chatroom = interestName.getSubName(chatroomBegin, i - chatroomBegin);

  _LOG_DEBUG(m_chatroom.toUri());

  string inviterStr("inviter");
  int inviterPrefixBegin = (++i);
  for(; i < size; i++)
    if(interestName.get(i).toUri() == inviterStr)
      break;
  
  if(i > size)
    throw LnException("Wrong ChronosInvitation Name, No inviter tag");
  m_inviterPrefix = interestName.getSubName(inviterPrefixBegin, i - inviterPrefixBegin);

  _LOG_DEBUG(m_inviterPrefix.toUri());

  int inviterCertBegin = (++i);
  m_inviterCertificateName = interestName.getSubName(inviterCertBegin, size - 1 - inviterCertBegin);

  _LOG_DEBUG(m_inviterCertificateName.toUri());
  
  string signature = interestName.get(-1).toBlob();
  m_signatureBits.insert(m_signatureBits.end(), signature.begin(), signature.end());

  string keyStr("KEY");
  int keyId = 0;
  for(; keyId < m_inviterCertificateName.size(); keyId++)
    if(m_inviterCertificateName.get(keyId).toUri() == keyStr)
      break;
  if(keyId >= m_inviterCertificateName.size())
    throw LnException("Wrong ChronosInvitation Name, no KEY tag in inviter Certificate Name");
  m_inviterNameSpace = m_inviterCertificateName.getSubName(0, keyId);

  string signedName = interestName.getSubName(0, size - 1).toUri();
  m_signedBlob.insert(m_signedBlob.end(), signedName.begin(), signedName.end());
}

ChronosInvitation::ChronosInvitation(const ChronosInvitation& invitation)
  : m_interestName(invitation.m_interestName)
  , m_inviteeNameSpace(invitation.m_inviteeNameSpace)
  , m_chatroom(invitation.m_chatroom)
  , m_inviterPrefix(invitation.m_inviterPrefix)
  , m_inviterCertificateName(invitation.m_inviterCertificateName)
  , m_signatureBits(invitation.m_signatureBits)
  , m_inviterNameSpace(invitation.m_inviterNameSpace)
  , m_signedBlob(invitation.m_signedBlob)
{}
