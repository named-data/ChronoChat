/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "controller-backend.hpp"

#ifndef Q_MOC_RUN
#include "invitation.hpp"
#include "logging.h"
#endif


INIT_LOGGER("ControllerBackend");

namespace chronos {

using std::string;

using ndn::Face;
using ndn::IdentityCertificate;
using ndn::OnInterestValidated;
using ndn::OnInterestValidationFailed;


static const uint8_t ROUTING_PREFIX_SEPARATOR[2] = {0xF0, 0x2E};

ControllerBackend::ControllerBackend(QObject* parent)
  : QThread(parent)
  , m_contactManager(m_face)
  , m_invitationListenerId(0)
{
  // connection to contact manager
  connect(this, SIGNAL(identityUpdated(const QString&)),
          &m_contactManager, SLOT(onIdentityUpdated(const QString&)));

  connect(&m_contactManager, SIGNAL(contactIdListReady(const QStringList&)),
          this, SLOT(onContactIdListReady(const QStringList&)));

}

ControllerBackend::~ControllerBackend()
{
}

void
ControllerBackend::run()
{
  setInvitationListener();

  m_face.processEvents();

  std::cerr << "Bye!" << std::endl;
}

// private methods:

void
tmpOnInvitationInterest(const ndn::Name& prefix,
                        const ndn::Interest& interest)
{
  std::cerr << "tmpOnInvitationInterest" << std::endl;
}

void
tmpOnInvitationRegisterFailed(const Name& prefix,
                              const string& failInfo)
{
  std::cerr << "tmpOnInvitationRegisterFailed" << std::endl;
}

void
tmpUnregisterPrefixSuccessCallback()
{
  std::cerr << "tmpUnregisterPrefixSuccessCallback" << std::endl;
}

void
tmpUnregisterPrefixFailureCallback(const string& failInfo)
{
  std::cerr << "tmpUnregisterPrefixSuccessCallback" << std::endl;
}

void
ControllerBackend::setInvitationListener()
{
  QMutexLocker locker(&m_mutex);

  Name invitationPrefix;
  Name routingPrefix = getInvitationRoutingPrefix();
  size_t offset = 0;
  if (!routingPrefix.isPrefixOf(m_identity)) {
    invitationPrefix.append(routingPrefix).append(ROUTING_PREFIX_SEPARATOR, 2);
    offset = routingPrefix.size() + 1;
  }
  invitationPrefix.append(m_identity).append("CHRONOCHAT-INVITATION");

  const ndn::RegisteredPrefixId* invitationListenerId =
    m_face.setInterestFilter(invitationPrefix,
                             bind(&ControllerBackend::onInvitationInterest,
                                  this, _1, _2, offset),
                             bind(&ControllerBackend::onInvitationRegisterFailed,
                                  this, _1, _2));

  if (m_invitationListenerId != 0) {
    m_face.unregisterPrefix(m_invitationListenerId,
                            bind(&ControllerBackend::onInvitationPrefixReset, this),
                            bind(&ControllerBackend::onInvitationPrefixResetFailed, this, _1));
  }

  m_invitationListenerId = invitationListenerId;

}

ndn::Name
ControllerBackend::getInvitationRoutingPrefix()
{
  return Name("/ndn/broadcast");
}

void
ControllerBackend::onInvitationPrefixReset()
{
  // _LOG_DEBUG("ControllerBackend::onInvitationPrefixReset");
}

void
ControllerBackend::onInvitationPrefixResetFailed(const std::string& failInfo)
{
  // _LOG_DEBUG("ControllerBackend::onInvitationPrefixResetFailed: " << failInfo);
}


void
ControllerBackend::onInvitationInterest(const ndn::Name& prefix,
                                        const ndn::Interest& interest,
                                        size_t routingPrefixOffset)
{
  // _LOG_DEBUG("onInvitationInterest: " << interest.getName());
  shared_ptr<Interest> invitationInterest =
    make_shared<Interest>(boost::cref(interest.getName().getSubName(routingPrefixOffset)));

  // check if the chatroom already exists;
  try {
      Invitation invitation(invitationInterest->getName());
      if (m_chatDialogList.contains(QString::fromStdString(invitation.getChatroom())))
        return;
  }
  catch (Invitation::Error& e) {
    // Cannot parse the invitation;
    return;
  }

  OnInterestValidated onValidated = bind(&ControllerBackend::onInvitationValidated, this, _1);
  OnInterestValidationFailed onFailed = bind(&ControllerBackend::onInvitationValidationFailed,
                                             this, _1, _2);

  m_validator.validate(*invitationInterest, onValidated, onFailed);
}

void
ControllerBackend::onInvitationRegisterFailed(const Name& prefix, const string& failInfo)
{
  // _LOG_DEBUG("ControllerBackend::onInvitationRegisterFailed: " << failInfo);
}

void
ControllerBackend::onInvitationValidated(const shared_ptr<const Interest>& interest)
{
  Invitation invitation(interest->getName());
  // Should be obtained via a method of ContactManager.
  string alias = invitation.getInviterCertificate().getPublicKeyName().getPrefix(-1).toUri();

  emit invitaionValidated(QString::fromStdString(alias),
                          QString::fromStdString(invitation.getChatroom()),
                          interest->getName());
}

void
ControllerBackend::onInvitationValidationFailed(const shared_ptr<const Interest>& interest,
                                                string failureInfo)
{
  // _LOG_DEBUG("Invitation: " << interest->getName() <<
  //            " cannot not be validated due to: " << failureInfo);
}

void
ControllerBackend::onLocalPrefix(const Interest& interest, Data& data)
{
  Name prefix;

  Block contentBlock = data.getContent();
  try {
    contentBlock.parse();

    for (Block::element_const_iterator it = contentBlock.elements_begin();
         it != contentBlock.elements_end(); it++) {
      Name candidate;
      candidate.wireDecode(*it);
      if (candidate.isPrefixOf(m_identity)) {
        prefix = candidate;
        break;
      }
    }

    if (prefix.empty()) {
      if (contentBlock.elements_begin() != contentBlock.elements_end())
        prefix.wireDecode(*contentBlock.elements_begin());
      else
        prefix = Name("/private/local");
    }
  }
  catch (Block::Error& e) {
    prefix = Name("/private/local");
  }

  updateLocalPrefix(prefix);
}

void
ControllerBackend::onLocalPrefixTimeout(const Interest& interest)
{
  Name localPrefix("/private/local");
  updateLocalPrefix(localPrefix);
}

void
ControllerBackend::updateLocalPrefix(const Name& localPrefix)
{
  if (m_localPrefix.empty() || m_localPrefix != localPrefix) {
    m_localPrefix = localPrefix;
    emit localPrefixUpdated(QString::fromStdString(localPrefix.toUri()));
  }
}

// public slots:
void
ControllerBackend::shutdown()
{
  m_face.getIoService().stop();
}

void
ControllerBackend::addChatroom(QString chatroom)
{
  m_chatDialogList.append(chatroom);
}

void
ControllerBackend::removeChatroom(QString chatroom)
{
  m_chatDialogList.removeAll(chatroom);
}

void
ControllerBackend::onUpdateLocalPrefixAction()
{
  // Name interestName();
  Interest interest("/localhop/ndn-autoconf/routable-prefixes");
  interest.setInterestLifetime(time::milliseconds(1000));
  interest.setMustBeFresh(true);

  m_face.expressInterest(interest,
                         bind(&ControllerBackend::onLocalPrefix, this, _1, _2),
                         bind(&ControllerBackend::onLocalPrefixTimeout, this, _1));
}

void
ControllerBackend::onIdentityChanged(const QString& identity)
{
  m_chatDialogList.clear();

  m_identity = Name(identity.toStdString());

  std::cerr << "ControllerBackend::onIdentityChanged: " << m_identity << std::endl;

  m_keyChain.createIdentity(m_identity);

  setInvitationListener();

  emit identityUpdated(identity);
}

void
ControllerBackend::onInvitationResponded(const ndn::Name& invitationName, bool accepted)
{
  shared_ptr<Data> response = make_shared<Data>();
  shared_ptr<IdentityCertificate> chatroomCert;

  // generate reply;
  if (accepted) {
    Name responseName = invitationName;
    responseName.append(m_localPrefix.wireEncode());

    response->setName(responseName);

    // We should create a particular certificate for this chatroom,
    //but let's use default one for now.
    chatroomCert
      = m_keyChain.getCertificate(m_keyChain.getDefaultCertificateNameForIdentity(m_identity));

    response->setContent(chatroomCert->wireEncode());
    response->setFreshnessPeriod(time::milliseconds(1000));
  }
  else {
    response->setName(invitationName);
    response->setFreshnessPeriod(time::milliseconds(1000));
  }

  m_keyChain.signByIdentity(*response, m_identity);

  // Check if we need a wrapper
  Name invitationRoutingPrefix = getInvitationRoutingPrefix();
  if (invitationRoutingPrefix.isPrefixOf(m_identity))
    m_face.put(*response);
  else {
    Name wrappedName;
    wrappedName.append(invitationRoutingPrefix)
      .append(ROUTING_PREFIX_SEPARATOR, 2)
      .append(response->getName());

    // _LOG_DEBUG("onInvitationResponded: prepare reply " << wrappedName);

    shared_ptr<Data> wrappedData = make_shared<Data>(wrappedName);
    wrappedData->setContent(response->wireEncode());
    wrappedData->setFreshnessPeriod(time::milliseconds(1000));

    m_keyChain.signByIdentity(*wrappedData, m_identity);
    m_face.put(*wrappedData);
  }

  Invitation invitation(invitationName);
  emit startChatroomOnInvitation(invitation, true);
}

void
ControllerBackend::onContactIdListReady(const QStringList& list)
{
  ContactList contactList;

  m_contactManager.getContactList(contactList);
  m_validator.cleanTrustAnchor();

  for (ContactList::const_iterator it  = contactList.begin(); it != contactList.end(); it++)
    m_validator.addTrustAnchor((*it)->getPublicKeyName(), (*it)->getPublicKey());

}


} // namespace chronos

#if WAF
#include "controller-backend.moc"
// #include "controller-backend.cpp.moc"
#endif
