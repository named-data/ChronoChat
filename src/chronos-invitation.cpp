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

#include <ndn-cpp-dev/security/identity-certificate.hpp>
#include <ndn-cpp-dev/security/signature-sha256-with-rsa.hpp>

#include "logging.h"

using namespace std;
using namespace ndn;

INIT_LOGGER("ChronosInvitation");


const size_t ChronosInvitation::NAME_SIZE_MIN = 8;
const size_t ChronosInvitation::INVITEE_START = 4;
const ssize_t SIGNATURE      = -1;
const ssize_t KEY_LOCATOR    = -2;
const ssize_t INVITER_PREFIX = -3;
const ssize_t CHATROOM       = -4;
const Name ChronosInvitation::INVITATION_PREFIX("/ndn/broadcast/chronos/chat-invitation");


ChronosInvitation::ChronosInvitation(const Name& originalInterestName)
  : m_interestName(originalInterestName)
{

  size_t nameSize = originalInterestName.size();

  if(nameSize < NAME_SIZE_MIN)
    throw Error("Wrong ChronosInvitation Name: Wrong length"); 
 
  if(!INVITATION_PREFIX.isPrefixOf(originalInterestName))
    throw Error("Wrong ChronosInvitation Name: Wrong invitation prefix");

  //hack! should be more efficient.
  Name signedName = originalInterestName.getPrefix(-1);
  m_signedBlob = Buffer(signedName.wireEncode().value(), signedName.wireEncode().value_size());

  Block signatureBlock(originalInterestName.get(SIGNATURE).getValue().buf(),
                       originalInterestName.get(SIGNATURE).getValue().size());
  Block signatureInfo(originalInterestName.get(KEY_LOCATOR).getValue().buf(),
                      originalInterestName.get(KEY_LOCATOR).getValue().size());
  m_signature = Signature(Signature(signatureInfo, signatureBlock));

  SignatureSha256WithRsa sha256RsaSig(m_signature); 
  m_inviterCertificateName = sha256RsaSig.getKeyLocator().getName();
  
  m_inviterNameSpace = IdentityCertificate::certificateNameToPublicKeyName(m_inviterCertificateName).getPrefix(-1);

  m_inviterRoutingPrefix.wireDecode(Block(originalInterestName.get(INVITER_PREFIX).getValue().buf(),
                                          originalInterestName.get(INVITER_PREFIX).getValue().size()));
  
  m_chatroom.wireDecode(Block(originalInterestName.get(CHATROOM).getValue().buf(),
                              originalInterestName.get(CHATROOM).getValue().size()));

  m_inviteeNameSpace = originalInterestName.getSubName(INVITEE_START, nameSize - NAME_SIZE_MIN);  
  
  m_isSigned = true;
}

ChronosInvitation::ChronosInvitation(const Name &inviteeNameSpace,
                                     const Name &chatroom,
                                     const Name &inviterRoutingPrefix,
                                     const Name &inviterCertificateName)
  : m_inviteeNameSpace(inviteeNameSpace)
  , m_chatroom(chatroom)
  , m_inviterRoutingPrefix(inviterRoutingPrefix)
  , m_inviterCertificateName(inviterCertificateName)
{
  //implicit conversion, we do not keep version number in KeyLocator;
  SignatureSha256WithRsa sha256RsaSig; 
  sha256RsaSig.setKeyLocator(KeyLocator(inviterCertificateName.getPrefix(-1)));
  m_signature.setInfo(sha256RsaSig.getInfo());
  m_inviterNameSpace = IdentityCertificate::certificateNameToPublicKeyName(m_inviterCertificateName).getPrefix(-1);
  
  m_interestName = INVITATION_PREFIX;
  m_interestName.append(inviteeNameSpace).append(chatroom.wireEncode()).append(inviterRoutingPrefix.wireEncode()).append(m_signature.getInfo());

  m_signedBlob = Buffer(m_interestName.wireEncode().value(), m_interestName.wireEncode().value_size());
  m_isSigned = false;
}

ChronosInvitation::ChronosInvitation(const ChronosInvitation& invitation)
  : m_interestName(invitation.m_interestName)
  , m_signedBlob(invitation.m_signedBlob)
  , m_inviteeNameSpace(invitation.m_inviteeNameSpace)
  , m_chatroom(invitation.m_chatroom)
  , m_inviterRoutingPrefix(invitation.m_inviterRoutingPrefix)
  , m_inviterCertificateName(invitation.m_inviterCertificateName)
  , m_signature(invitation.m_signature)
  , m_inviterNameSpace(invitation.m_inviterNameSpace)
  , m_isSigned(invitation.m_isSigned)
{}

void
ChronosInvitation::setSignatureValue(const ndn::Block &signatureValue)
{
  if(m_isSigned)
    return;

  m_interestName.append(signatureValue);
  m_signature.setValue(signatureValue);
  m_isSigned = true;
}
