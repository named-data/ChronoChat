/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013-2016, Regents of the University of California
 *                          Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 *         Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#include "controller-backend.hpp"

#ifndef Q_MOC_RUN
#include <ndn-cxx/util/segment-fetcher.hpp>
#include "invitation.hpp"
#include "logging.h"
#endif


INIT_LOGGER("ControllerBackend");

namespace chronochat {

using std::string;

using ndn::Face;
using ndn::IdentityCertificate;
using ndn::OnInterestValidated;
using ndn::OnInterestValidationFailed;


static const ndn::Name::Component ROUTING_HINT_SEPARATOR =
  ndn::name::Component::fromEscapedString("%F0%2E");
static const int MAXIMUM_REQUEST = 3;
static const int CONNECTION_RETRY_TIMER = 3;

ControllerBackend::ControllerBackend(QObject* parent)
  : QThread(parent)
  , m_shouldResume(false)
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
  bool shouldResume = false;
  do {
    try {
      setInvitationListener();
      m_face.processEvents();
    }
    catch (std::runtime_error& e) {
      {
        std::lock_guard<std::mutex>lock(m_nfdConnectionMutex);
        m_isNfdConnected = false;
      }
      emit nfdError();
      {
        std::lock_guard<std::mutex>lock(m_resumeMutex);
        m_shouldResume = true;
      }
#ifdef BOOST_THREAD_USES_CHRONO
      time::seconds reconnectTimer = time::seconds(CONNECTION_RETRY_TIMER);
#else
      boost::posix_time::time_duration reconnectTimer;
      reconnectTimer = boost::posix_time::seconds(CONNECTION_RETRY_TIMER);
#endif
      while (!m_isNfdConnected) {
#ifdef BOOST_THREAD_USES_CHRONO
        boost::this_thread::sleep_for(reconnectTimer);
#else
        boost::this_thread::sleep(reconnectTimer);
#endif
      }
    }
    {
      std::lock_guard<std::mutex>lock(m_resumeMutex);
      shouldResume = m_shouldResume;
      m_shouldResume = false;
    }
  } while (shouldResume);
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
  Name requestPrefix;
  Name routingPrefix = getInvitationRoutingPrefix();
  size_t offset = 0;
  if (!routingPrefix.isPrefixOf(m_identity)) {
    invitationPrefix.append(routingPrefix).append(ROUTING_HINT_SEPARATOR);
    requestPrefix.append(routingPrefix).append(ROUTING_HINT_SEPARATOR);
    offset = routingPrefix.size() + 1;
  }
  invitationPrefix.append(m_identity).append("CHRONOCHAT-INVITATION");
  requestPrefix.append(m_identity).append("CHRONOCHAT-INVITATION-REQUEST");

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

  const ndn::RegisteredPrefixId* requestListenerId =
    m_face.setInterestFilter(requestPrefix,
                             bind(&ControllerBackend::onInvitationRequestInterest,
                                  this, _1, _2, offset),
                             [] (const Name& prefix, const std::string& failInfo) {});

  if (m_requestListenerId != 0) {
    m_face.unregisterPrefix(m_requestListenerId,
                            []{},
                            [] (const std::string& failInfo) {});
  }

  m_requestListenerId = requestListenerId;
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
    make_shared<Interest>(interest.getName().getSubName(routingPrefixOffset));

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
ControllerBackend::onInvitationRegisterFailed(const Name& prefix, const std::string& failInfo)
{
  // _LOG_DEBUG("ControllerBackend::onInvitationRegisterFailed: " << failInfo);
}

void
ControllerBackend::onInvitationRequestInterest(const ndn::Name& prefix,
                                               const ndn::Interest& interest,
                                               size_t routingPrefixOffset)
{
  shared_ptr<const Data> data = m_ims.find(interest);
  if (data != nullptr) {
    m_face.put(*data);
    return;
  }
  Name interestName = interest.getName();
  size_t i;
  for (i = 0; i < interestName.size(); i++)
    if (interestName.at(i) == Name::Component("CHRONOCHAT-INVITATION-REQUEST"))
      break;
  if (i < interestName.size()) {
    string chatroom = interestName.at(i+1).toUri();
    string alias = interestName.getSubName(i+2).getPrefix(-1).toUri();
    emit invitationRequestReceived(QString::fromStdString(alias),
                                   QString::fromStdString(chatroom),
                                   interestName);
  }
}

void
ControllerBackend::onInvitationValidated(const shared_ptr<const Interest>& interest)
{
  Invitation invitation(interest->getName());
  // Should be obtained via a method of ContactManager.
  string alias = invitation.getInviterCertificate().getPublicKeyName().getPrefix(-1).toUri();

  emit invitationValidated(QString::fromStdString(alias),
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
ControllerBackend::onLocalPrefix(const ndn::ConstBufferPtr& data)
{
  Name prefix;

  Block contentBlock(tlv::Content, data);
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
ControllerBackend::onLocalPrefixError(uint32_t code, const std::string& msg)
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

void
ControllerBackend::onRequestResponse(const Interest& interest, Data& data)
{
  size_t i;
  Name interestName = interest.getName();
  for (i = 0; i < interestName.size(); i++) {
    if (interestName.at(i) == Name::Component("CHRONOCHAT-INVITATION-REQUEST"))
      break;
  }
  Name::Component chatroomName = interestName.at(i+1);
  Block contentBlock = data.getContent();
  int res = ndn::readNonNegativeInteger(contentBlock);
  if (m_chatDialogList.contains(QString::fromStdString(chatroomName.toUri())))
    return;

  // if data is true,
  if (res == 1)
    emit startChatroom(QString::fromStdString(chatroomName.toUri()), false);
  else
    emit invitationRequestResult("You are rejected to enter chatroom: " + chatroomName.toUri());
}

void
ControllerBackend::onRequestTimeout(const Interest& interest, int& resendTimes)
{
  if (resendTimes < MAXIMUM_REQUEST)
    m_face.expressInterest(interest,
                           bind(&ControllerBackend::onRequestResponse, this, _1, _2),
                           bind(&ControllerBackend::onRequestTimeout, this, _1, resendTimes + 1));
  else
    emit invitationRequestResult("Invitation request times out.");
}

// public slots:
void
ControllerBackend::shutdown()
{
  {
    std::lock_guard<std::mutex>lock(m_resumeMutex);
    m_shouldResume = false;
  }
  {
    // In this case, we just stop checking the nfd connection and exit
    std::lock_guard<std::mutex>lock(m_nfdConnectionMutex);
    m_isNfdConnected = true;
  }
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
  Interest interest("/localhop/nfd/rib/routable-prefixes");
  interest.setInterestLifetime(time::milliseconds(1000));
  interest.setMustBeFresh(true);

  ndn::util::SegmentFetcher::fetch(m_face,
                                   interest,
                                   m_nullValidator,
                                   bind(&ControllerBackend::onLocalPrefix, this, _1),
                                   bind(&ControllerBackend::onLocalPrefixError, this, _1, _2));
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
      .append(ROUTING_HINT_SEPARATOR)
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
ControllerBackend::onInvitationRequestResponded(const ndn::Name& invitationResponseName,
                                                bool accepted)
{
  shared_ptr<Data> response = make_shared<Data>(invitationResponseName);
  if (accepted)
    response->setContent(ndn::makeNonNegativeIntegerBlock(tlv::Content, 1));
  else
    response->setContent(ndn::makeNonNegativeIntegerBlock(tlv::Content, 0));

  m_keyChain.signByIdentity(*response, m_identity);
  m_ims.insert(*response);
  m_face.put(*response);
}

void
ControllerBackend::onSendInvitationRequest(const QString& chatroomName, const QString& prefix)
{
  if (prefix.length() == 0)
    return;
  Name interestName = getInvitationRoutingPrefix();
  interestName.append(ROUTING_HINT_SEPARATOR).append(prefix.toStdString());
  interestName.append("CHRONOCHAT-INVITATION-REQUEST");
  interestName.append(chatroomName.toStdString());
  interestName.append(m_identity);
  interestName.appendTimestamp();
  Interest interest(interestName);
  interest.setInterestLifetime(time::milliseconds(10000));
  interest.setMustBeFresh(true);
  interest.getNonce();
  m_face.expressInterest(interest,
                         bind(&ControllerBackend::onRequestResponse, this, _1, _2),
                         bind(&ControllerBackend::onRequestTimeout, this, _1, 0));
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

void
ControllerBackend::onNfdReconnect()
{
  std::lock_guard<std::mutex>lock(m_nfdConnectionMutex);
  m_isNfdConnected = true;
}


} // namespace chronochat

#if WAF
#include "controller-backend.moc"
// #include "controller-backend.cpp.moc"
#endif
