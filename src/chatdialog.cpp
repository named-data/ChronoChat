/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "chatdialog.h"
#include "ui_chatdialog.h"

#ifndef Q_MOC_RUN
#include <ndn.cxx/security/identity/identity-manager.h>
#include <ndn.cxx/security/identity/basic-identity-storage.h>
#include <ndn.cxx/security/identity/osx-privatekey-storage.h>
#include <ndn.cxx/security/encryption/basic-encryption-manager.h>
#include "logging.h"
#endif

using namespace std;
using namespace ndn;

INIT_LOGGER("ChatDialog");

ChatDialog::ChatDialog(const Name& chatroomPrefix,
		       const Name& localPrefix,
                       const Name& defaultIdentity,
		       QWidget *parent) 
    : QDialog(parent)
    , m_chatroomPrefix(chatroomPrefix)
    , m_localPrefix(localPrefix)
    , m_defaultIdentity(defaultIdentity)
    , m_policyManager(Ptr<ChatroomPolicyManager>(new ChatroomPolicyManager))
    , ui(new Ui::ChatDialog)
{
  ui->setupUi(this);

  setWrapper();
}

ChatDialog::~ChatDialog()
{
  delete ui;
  m_handler->shutdown();
}

void
ChatDialog::setWrapper()
{
  Ptr<security::OSXPrivatekeyStorage> privateStorage = Ptr<security::OSXPrivatekeyStorage>::Create();
  m_identityManager = Ptr<security::IdentityManager>(new security::IdentityManager(Ptr<security::BasicIdentityStorage>::Create(), privateStorage));
  Ptr<security::EncryptionManager> encryptionManager = Ptr<security::EncryptionManager>(new security::BasicEncryptionManager(privateStorage, "/tmp/encryption.db"));

  m_keychain = Ptr<security::Keychain>(new security::Keychain(m_identityManager, m_policyManager, encryptionManager));

  m_handler = Ptr<Wrapper>(new Wrapper(m_keychain));
}

void
ChatDialog::sendInvitation(Ptr<ContactItem> contact)
{
  m_policyManager->addTrustAnchor(contact->getSelfEndorseCertificate());

  Name certificateName = m_identityManager->getDefaultCertificateNameByIdentity(m_defaultIdentity);

  Name interestName("/ndn/broadcast/chronos/invitation");
  interestName.append(contact->getNameSpace());
  interestName.append("chatroom");
  interestName.append(m_chatroomPrefix.get(-1));
  interestName.append("inviter-prefix");
  interestName.append(m_localPrefix);
  interestName.append("inviter");
  interestName.append(certificateName);

  string signedUri = interestName.toUri();
  Blob signedBlob(signedUri.c_str(), signedUri.size());

  Ptr<const signature::Sha256WithRsa> sha256sig = DynamicCast<const signature::Sha256WithRsa>(m_identityManager->signByCertificate(signedBlob, certificateName));
  const Blob& sigBits = sha256sig->getSignatureBits();

  interestName.append(sigBits.buf(), sigBits.size());

  Ptr<Interest> interest = Ptr<Interest>(new Interest(interestName));
  Ptr<Closure> closure = Ptr<Closure>(new Closure(boost::bind(&ChatDialog::onInviteReplyVerified,
                                                              this,
                                                              _1,
                                                              contact->getNameSpace()),
                                                  boost::bind(&ChatDialog::onInviteTimeout,
                                                              this,
                                                              _1,
                                                              _2,
                                                              contact->getNameSpace(),
                                                              7),
                                                  boost::bind(&ChatDialog::onUnverified,
                                                              this,
                                                              _1)));

  m_handler->sendInterest(interest, closure);
}

void
ChatDialog::invitationRejected(const Name& identity)
{
  _LOG_DEBUG(" " << identity.toUri() << " rejected your invitation!");
}

void
ChatDialog::invitationAccepted(const Name& identity)
{
  _LOG_DEBUG(" " << identity.toUri() << " accepted your invitation!");
}

void 
ChatDialog::onInviteReplyVerified(Ptr<Data> data, const Name& identity)
{
  string content(data->content().buf(), data->content().size());
  if(content.empty())
    invitationRejected(identity);
  else
    invitationAccepted(identity);
}

void 
ChatDialog::onInviteTimeout(Ptr<Closure> closure, Ptr<Interest> interest, const Name& identity, int retry)
{
  if(retry > 0)
    {
      Ptr<Closure> newClosure = Ptr<Closure>(new Closure(closure->m_dataCallback,
                                                         boost::bind(&ChatDialog::onInviteTimeout, 
                                                                     this, 
                                                                     _1, 
                                                                     _2, 
                                                                     identity,
                                                                     retry - 1),
                                                         closure->m_unverifiedCallback,
                                                         closure->m_stepCount)
                                             );
      m_handler->sendInterest(interest, newClosure);
    }
  else
    invitationRejected(identity);
}
 
void
ChatDialog::onUnverified(Ptr<Data> data)
{}

#if WAF
#include "chatdialog.moc"
#include "chatdialog.cpp.moc"
#endif
