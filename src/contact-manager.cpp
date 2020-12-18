/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013-2020, Regents of the University of California
 *                          Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 *         Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#include "contact-manager.hpp"

#include <QStringList>
#include <QFile>

#ifndef Q_MOC_RUN
#include <ndn-cxx/encoding/buffer-stream.hpp>
#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/security/transform/buffer-source.hpp>
#include <ndn-cxx/security/transform/digest-filter.hpp>
#include <ndn-cxx/security/transform/stream-sink.hpp>
#include <ndn-cxx/security/validator.hpp>
#include <ndn-cxx/security/validator-null.hpp>
#include <ndn-cxx/security/verification-helpers.hpp>

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#endif

namespace fs = boost::filesystem;

namespace chronochat {

using std::string;
using std::map;
using std::vector;

using ndn::Face;
using ndn::OBufferStream;
using ndn::security::Certificate;


ContactManager::ContactManager(Face& face,
                               QObject* parent)
  : QObject(parent)
  , m_face(face)
{
  initializeSecurity();
}

ContactManager::~ContactManager()
{
}

void
ContactManager::initializeSecurity()
{
  m_validator = make_shared<ndn::security::ValidatorConfig>(m_face);
  m_validator->load("security/validation-contact-manager.conf");
}

void
ContactManager::fetchCollectEndorse(const Name& identity)
{
  Name interestName = identity;
  interestName.append("DNS").append("ENDORSED");

  Interest interest(interestName);
  interest.setInterestLifetime(time::milliseconds(1000));
  interest.setCanBePrefix(true);
  interest.setMustBeFresh(true);

  ndn::security::DataValidationSuccessCallback onValidated =
    bind(&ContactManager::onDnsCollectEndorseValidated, this, _1, identity);
  ndn::security::DataValidationFailureCallback onValidationFailed =
    bind(&ContactManager::onDnsCollectEndorseValidationFailed, this, _1, _2, identity);
  TimeoutNotify timeoutNotify =
    bind(&ContactManager::onDnsCollectEndorseTimeoutNotify, this, _1, identity);

  sendInterest(interest, onValidated, onValidationFailed, timeoutNotify, 0);
}

void
ContactManager::fetchEndorseCertificateInternal(const Name& identity, size_t certIndex)
{
  auto endorseCollection = m_bufferedContacts[identity].m_endorseCollection;

  if (certIndex >= endorseCollection->getCollectionEntries().size())
    return prepareEndorseInfo(identity);

  Name interestName(endorseCollection->getCollectionEntries()[certIndex].certName);

  Interest interest(interestName);
  interest.setInterestLifetime(time::milliseconds(1000));
  interest.setCanBePrefix(true);
  interest.setMustBeFresh(false);

  m_face.expressInterest(interest,
                         bind(&ContactManager::onEndorseCertificateInternal,
                              this, _1, _2, identity, certIndex,
                              endorseCollection->getCollectionEntries()[certIndex].hash),
                         bind(&ContactManager::onEndorseCertificateInternalTimeout,
                              this, _1, identity, certIndex),
                         bind(&ContactManager::onEndorseCertificateInternalTimeout,
                              this, _1, identity, certIndex));
}

void
ContactManager::prepareEndorseInfo(const Name& identity)
{
  const Profile& profile = m_bufferedContacts[identity].m_selfEndorseCert->getProfile();

  shared_ptr<EndorseInfo> endorseInfo = make_shared<EndorseInfo>();
  m_bufferedContacts[identity].m_endorseInfo = endorseInfo;

  map<string, size_t> endorseCount;
  for (auto pIt = profile.begin(); pIt != profile.end(); pIt++)
    endorseCount[pIt->first] = 0;

  size_t endorseCertCount = 0;

  auto cIt = m_bufferedContacts[identity].m_endorseCertList.cbegin();
  auto cEnd = m_bufferedContacts[identity].m_endorseCertList.cend();

  for (; cIt != cEnd; cIt++, endorseCertCount++) {
    shared_ptr<Contact> contact = getContact((*cIt)->getSigner());
    if (!static_cast<bool>(contact))
      continue;

    if (!contact->isIntroducer() ||
        !contact->canBeTrustedFor(profile.getIdentityName()))
      continue;

    if (!(*cIt)->isValid())
      continue;

    if (!ndn::security::verifySignature(**cIt, contact->getPublicKey().data(), contact->getPublicKey().size()))
      continue;

    const Profile& tmpProfile = (*cIt)->getProfile();
    const auto& endorseList = (*cIt)->getEndorseList();
    for (auto eIt = endorseList.begin(); eIt != endorseList.end(); eIt++)
      if (tmpProfile.get(*eIt) == profile.get(*eIt))
        endorseCount[*eIt] += 1;
  }

  for (auto pIt = profile.begin(); pIt != profile.end(); pIt++) {
    std::stringstream ss;
    ss << endorseCount[pIt->first] << "/" << endorseCertCount;
    endorseInfo->addEndorsement(pIt->first, pIt->second, ss.str());
  }

  emit contactEndorseInfoReady (*endorseInfo);
}

void
ContactManager::onDnsSelfEndorseCertValidated(const Data& data,
                                              const Name& identity)
{
  try {
    Data plainData;
    plainData.wireDecode(data.getContent().blockFromValue());
    shared_ptr<EndorseCertificate> selfEndorseCertificate =
      make_shared<EndorseCertificate>(boost::cref(plainData));

    if (ndn::security::verifySignature(plainData, *selfEndorseCertificate)) {
      m_bufferedContacts[identity].m_selfEndorseCert = selfEndorseCertificate;
      fetchCollectEndorse(identity);
    }
    else
      emit contactInfoFetchFailed(QString::fromStdString(identity.toUri() + ": verification failed"));
  }
  catch (const Block::Error& e) {
    emit contactInfoFetchFailed(QString::fromStdString(identity.toUri() + ": block error " + e.what()));
  }
  catch (const EndorseCertificate::Error& e) {
    emit contactInfoFetchFailed(QString::fromStdString(identity.toUri() + ": cert error " + e.what()));
  }
  catch (const Data::Error& e) {
    emit contactInfoFetchFailed(QString::fromStdString(identity.toUri() + ": data error " + e.what()));
  }
}

void
ContactManager::onDnsSelfEndorseCertValidationFailed(const Data& data,
                                                     const ndn::security::ValidationError& error,
                                                     const Name& identity)
{
  // If we cannot validate the Self-Endorse-Certificate, we may retry or fetch id-cert,
  // but let's stay with failure for now.
  emit contactInfoFetchFailed(QString::fromStdString(identity.toUri()));
}

void
ContactManager::onDnsSelfEndorseCertTimeoutNotify(const Interest& interest,
                                                  const Name& identity)
{
  // If we cannot validate the Self-Endorse-Certificate, we may retry or fetch id-cert,
  // but let's stay with failure for now.
  emit contactInfoFetchFailed(QString::fromStdString(identity.toUri()));
}

void
ContactManager::onDnsCollectEndorseValidated(const Data& data,
                                             const Name& identity)
{
  try {
    shared_ptr<EndorseCollection> endorseCollection =
      make_shared<EndorseCollection>(data.getContent().blockFromValue());
    m_bufferedContacts[identity].m_endorseCollection = endorseCollection;
    fetchEndorseCertificateInternal(identity, 0);
  }
  catch (const std::runtime_error&) {
    prepareEndorseInfo(identity);
  }
}

void
ContactManager::onDnsCollectEndorseValidationFailed(const Data& data,
                                                    const ndn::security::ValidationError& error,
                                                    const Name& identity)
{
  prepareEndorseInfo(identity);
}

void
ContactManager::onDnsCollectEndorseTimeoutNotify(const Interest& interest, const Name& identity)
{
  prepareEndorseInfo(identity);
}

void
ContactManager::onEndorseCertificateInternal(const Interest&, const Data& data,
                                             const Name& identity, size_t certIndex, string hash)
{
  std::ostringstream ss;
  {
    using namespace ndn::security::transform;
    bufferSource(data.wireEncode().wire(), data.wireEncode().size())
        >> digestFilter(ndn::DigestAlgorithm::SHA256)
        >> streamSink(ss);
  }

  if (ss.str() == hash) {
    auto endorseCertificate = make_shared<EndorseCertificate>(data);
    m_bufferedContacts[identity].m_endorseCertList.push_back(std::move(endorseCertificate));
  }

  fetchEndorseCertificateInternal(identity, certIndex+1);
}

void
ContactManager::onEndorseCertificateInternalTimeout(const Interest& interest,
                                                    const Name& identity,
                                                    size_t certIndex)
{
  fetchEndorseCertificateInternal(identity, certIndex+1);
}

void
ContactManager::collectEndorsement()
{
  {
    boost::recursive_mutex::scoped_lock lock(m_collectCountMutex);
    m_collectCount = m_contactList.size();

    for (auto it = m_contactList.begin(); it != m_contactList.end(); it++) {
      Name interestName = (*it)->getNameSpace();
      interestName.append("DNS").append(m_identity.wireEncode()).append("ENDORSEE");

      Interest interest(interestName);
      interest.setMustBeFresh(true);
      interest.setCanBePrefix(true);
      interest.setInterestLifetime(time::milliseconds(1000));

      ndn::security::DataValidationSuccessCallback onValidated =
        bind(&ContactManager::onDnsEndorseeValidated, this, _1);
      ndn::security::DataValidationFailureCallback onValidationFailed =
        bind(&ContactManager::onDnsEndorseeValidationFailed, this, _1, _2);
      TimeoutNotify timeoutNotify = bind(&ContactManager::onDnsEndorseeTimeoutNotify, this, _1);

      sendInterest(interest, onValidated, onValidationFailed, timeoutNotify, 0);
    }
  }
}

void
ContactManager::onDnsEndorseeValidated(const Data& data)
{
  Data endorseData;
  endorseData.wireDecode(data.getContent().blockFromValue());

  EndorseCertificate endorseCertificate(endorseData);
  m_contactStorage->updateCollectEndorse(endorseCertificate);

  decreaseCollectStatus();
}

void
ContactManager::onDnsEndorseeValidationFailed(const Data& data,
                                              const ndn::security::ValidationError& error)
{
  decreaseCollectStatus();
}

void
ContactManager::onDnsEndorseeTimeoutNotify(const Interest& interest)
{
  decreaseCollectStatus();
}

void
ContactManager::decreaseCollectStatus()
{
  size_t count;
  {
    boost::recursive_mutex::scoped_lock lock(m_collectCountMutex);
    m_collectCount--;
    count = m_collectCount;
  }

  if(count == 0)
    publishCollectEndorsedDataInDNS();
}

void
ContactManager::publishCollectEndorsedDataInDNS()
{
  Name dnsName = m_identity;
  dnsName.append("DNS").append("ENDORSED").appendVersion();

  shared_ptr<Data> data = make_shared<Data>();
  data->setName(dnsName);
  data->setFreshnessPeriod(time::milliseconds(1000));

  EndorseCollection endorseCollection;
  m_contactStorage->getCollectEndorse(endorseCollection);

  data->setContent(endorseCollection.wireEncode());

  m_keyChain.sign(*data, ndn::security::signingByIdentity(m_identity));

  m_contactStorage->updateDnsOthersEndorse(*data);
  m_face.put(*data);
}

void
ContactManager::onIdentityCertValidated(const Data& data)
{
  shared_ptr<Certificate> cert = make_shared<Certificate>(boost::cref(data));
  m_bufferedIdCerts[cert->getName()] = cert;
  decreaseIdCertCount();
}

void
ContactManager::onIdentityCertValidationFailed(const Data& data,
                                               const ndn::security::ValidationError& error)
{
  decreaseIdCertCount();
}

void
ContactManager::onIdentityCertTimeoutNotify(const Interest& interest)
{
  decreaseIdCertCount();
}

void
ContactManager::decreaseIdCertCount()
{
  size_t count;
  {
    boost::recursive_mutex::scoped_lock lock(m_idCertCountMutex);
    m_idCertCount--;
    count = m_idCertCount;
  }

  if (count == 0) {
    QStringList certNameList;
    QStringList nameList;

    for (auto it = m_bufferedIdCerts.begin(); it != m_bufferedIdCerts.end(); it++) {
      certNameList << QString::fromStdString(it->second->getName().toUri());
      Profile profile(*(it->second));
      nameList << QString::fromStdString(profile.get("name"));
    }

    emit idCertNameListReady(certNameList);
    emit nameListReady(nameList);
  }
}

shared_ptr<EndorseCertificate>
ContactManager::getSignedSelfEndorseCertificate(const Profile& profile)
{
  auto signCert = m_keyChain.getPib().getIdentity(m_identity)
                            .getDefaultKey().getDefaultCertificate();
  vector<string> endorseList;
  for (auto it = profile.begin(); it != profile.end(); it++)
    endorseList.push_back(it->first);

  shared_ptr<EndorseCertificate> selfEndorseCertificate =
    make_shared<EndorseCertificate>(boost::cref(signCert),
                                    boost::cref(profile),
                                    boost::cref(endorseList));

  m_keyChain.sign(*selfEndorseCertificate,
                  ndn::security::signingByIdentity(m_identity).setSignatureInfo(
                    selfEndorseCertificate->getSignatureInfo()));

  return selfEndorseCertificate;
}

void
ContactManager::publishSelfEndorseCertificateInDNS(const EndorseCertificate& selfEndorseCertificate)
{
  Name dnsName = m_identity;
  dnsName.append("DNS").append("PROFILE").appendVersion();

  shared_ptr<Data> data = make_shared<Data>();
  data->setName(dnsName);
  data->setContent(selfEndorseCertificate.wireEncode());
  data->setFreshnessPeriod(time::milliseconds(1000));

  m_keyChain.sign(*data, ndn::security::signingByIdentity(m_identity));

  m_contactStorage->updateDnsSelfProfileData(*data);
  m_face.put(*data);
}

shared_ptr<EndorseCertificate>
ContactManager::generateEndorseCertificate(const Name& identity)
{
  auto signCert = m_keyChain.getPib().getIdentity(m_identity)
                            .getDefaultKey().getDefaultCertificate();

  shared_ptr<Contact> contact = getContact(identity);
  if (!static_cast<bool>(contact))
    return shared_ptr<EndorseCertificate>();

  Name signerKeyName = m_identity;

  vector<string> endorseList;
  m_contactStorage->getEndorseList(identity, endorseList);

  shared_ptr<EndorseCertificate> cert =
    shared_ptr<EndorseCertificate>(new EndorseCertificate(contact->getPublicKeyName(),
                                                          contact->getPublicKey(),
                                                          contact->getNotBefore(),
                                                          contact->getNotAfter(),
                                                          signCert.getKeyId(),
                                                          signerKeyName,
                                                          contact->getProfile(),
                                                          endorseList));
  m_keyChain.sign(*cert,
                  ndn::security::signingByIdentity(m_identity)
                    .setSignatureInfo(cert->getSignatureInfo()));
  return cert;

}

void
ContactManager::publishEndorseCertificateInDNS(const EndorseCertificate& endorseCertificate)
{
  Name endorsee = endorseCertificate.getKeyName().getPrefix(-4);
  Name dnsName = m_identity;
  dnsName.append("DNS")
    .append(endorsee.wireEncode())
    .append("ENDORSEE")
    .appendVersion();

  shared_ptr<Data> data = make_shared<Data>();
  data->setName(dnsName);
  data->setContent(endorseCertificate.wireEncode());
  data->setFreshnessPeriod(time::milliseconds(1000));

  m_keyChain.sign(*data, ndn::security::signingByIdentity(m_identity));

  m_contactStorage->updateDnsEndorseOthers(*data, dnsName.get(-3).toUri());
  m_face.put(*data);
}

void
ContactManager::sendInterest(const Interest& interest,
                             const ndn::security::DataValidationSuccessCallback& onValidated,
                             const ndn::security::DataValidationFailureCallback& onValidationFailed,
                             const TimeoutNotify& timeoutNotify,
                             int retry /* = 1 */)
{
  m_face.expressInterest(interest,
                         bind(&ContactManager::onTargetData,
                              this, _1, _2, onValidated, onValidationFailed),
                         bind(&ContactManager::onTargetTimeout,
                              this, _1, retry, onValidated, onValidationFailed, timeoutNotify),
                         bind(&ContactManager::onTargetTimeout,
                              this, _1, retry, onValidated, onValidationFailed, timeoutNotify));
}

void
ContactManager::onTargetData(const Interest& interest,
                             const Data& data,
                             const ndn::security::DataValidationSuccessCallback& onValidated,
                             const ndn::security::DataValidationFailureCallback& onValidationFailed)
{
  m_validator->validate(data, onValidated, onValidationFailed);
}

void
ContactManager::onTargetTimeout(const Interest& interest,
                                int retry,
                                const ndn::security::DataValidationSuccessCallback& onValidated,
                                const ndn::security::DataValidationFailureCallback& onValidationFailed,
                                const TimeoutNotify& timeoutNotify)
{
  if (retry > 0)
    sendInterest(interest, onValidated, onValidationFailed, timeoutNotify, retry-1);
  else
    timeoutNotify(interest);
}

void
ContactManager::onDnsInterest(const Name& prefix, const Interest& interest)
{
  const Name& interestName = interest.getName();
  shared_ptr<Data> data;

  if (interestName.size() <= prefix.size())
    return;

  if (interestName.size() == (prefix.size()+1)) {
    data = m_contactStorage->getDnsData("N/A", interestName.get(prefix.size()).toUri());
    if (static_cast<bool>(data))
      m_face.put(*data);
    return;
  }

  if (interestName.size() == (prefix.size()+2)) {
    data = m_contactStorage->getDnsData(interestName.get(prefix.size()).toUri(),
                                        interestName.get(prefix.size()+1).toUri());
    if (static_cast<bool>(data))
      m_face.put(*data);
    return;
  }
}

void
ContactManager::onDnsRegisterFailed(const Name& prefix, const std::string& failInfo)
{
  emit warning(QString(failInfo.c_str()));
}

void
ContactManager::onKeyInterest(const Name& prefix, const Interest& interest)
{
  const Name& interestName = interest.getName();
  shared_ptr<Certificate> data;

  try {
    ndn::security::Certificate cert = m_keyChain.getPib()
                                                .getIdentity(m_identity)
                                                .getDefaultKey()
                                                .getDefaultCertificate();
    if (cert.getKeyName() == interestName)
      return m_face.put(cert);
  } catch (const ndn::security::Pib::Error&) {}

  data = m_contactStorage->getSelfEndorseCertificate();
  if (static_cast<bool>(data) && data->getKeyName().equals(interestName))
    return m_face.put(*data);

  data = m_contactStorage->getCollectEndorseByName(interestName);
  if (static_cast<bool>(data))
    return m_face.put(*data);
}

// public slots
void
ContactManager::onIdentityUpdated(const QString& identity)
{
  m_identity = Name(identity.toStdString());

  m_contactStorage = make_shared<ContactStorage>(m_identity);

  m_dnsListenerHandle = m_face.setInterestFilter(
    Name(m_identity).append("DNS"),
    bind(&ContactManager::onDnsInterest, this, _1, _2),
    bind(&ContactManager::onDnsRegisterFailed, this, _1, _2));

  m_keyListenerHandle = m_face.setInterestFilter(
    Name(m_identity).append("KEY"),
    bind(&ContactManager::onKeyInterest, this, _1, _2),
    bind(&ContactManager::onDnsRegisterFailed, this, _1, _2));

  m_profileCertListenerHandle = m_face.setInterestFilter(
    Name(m_identity).append("PROFILE-CERT"),
    bind(&ContactManager::onKeyInterest, this, _1, _2),
    bind(&ContactManager::onDnsRegisterFailed, this, _1, _2));

  m_contactList.clear();
  m_contactStorage->getAllContacts(m_contactList);

  m_bufferedContacts.clear();
  onWaitForContactList();

  collectEndorsement();
}

void
ContactManager::onFetchContactInfo(const QString& identity)
{
  // try to fetch self-endorse-certificate via DNS PROFILE first.
  Name identityName(identity.toStdString());
  Name interestName;
  interestName.append(identityName).append("DNS").append("PROFILE");

  Interest interest(interestName);
  interest.setInterestLifetime(time::milliseconds(1000));
  interest.setCanBePrefix(true);
  interest.setMustBeFresh(true);

  ndn::security::DataValidationSuccessCallback onValidated =
    bind(&ContactManager::onDnsSelfEndorseCertValidated, this, _1, identityName);
  ndn::security::DataValidationFailureCallback onValidationFailed =
    bind(&ContactManager::onDnsSelfEndorseCertValidationFailed, this, _1, _2, identityName);
  TimeoutNotify timeoutNotify =
    bind(&ContactManager::onDnsSelfEndorseCertTimeoutNotify, this, _1, identityName);

  sendInterest(interest, onValidated, onValidationFailed, timeoutNotify, 0);
}

void
ContactManager::onAddFetchedContact(const QString& identity)
{
  Name identityName(identity.toStdString());

  auto it = m_bufferedContacts.find(identityName);
  if (it != m_bufferedContacts.end()) {
    Contact contact(*(it->second.m_selfEndorseCert));

    try {
      m_contactStorage->addContact(contact);
      m_bufferedContacts.erase(identityName);

      m_contactList.clear();
      m_contactStorage->getAllContacts(m_contactList);

      onWaitForContactList();
    }
    catch (const ContactStorage::Error& e) {
      emit warning(QString::fromStdString(e.what()));
    }
  }
  else
    emit warning(QString("Failure: no information of %1").arg(identity));
}

void
ContactManager::onUpdateProfile()
{
  // Get current profile;
  shared_ptr<Profile> newProfile = m_contactStorage->getSelfProfile();
  if (!static_cast<bool>(newProfile))
    return;

  shared_ptr<EndorseCertificate> newEndorseCertificate =
    getSignedSelfEndorseCertificate(*newProfile);

  m_contactStorage->addSelfEndorseCertificate(*newEndorseCertificate);

  publishSelfEndorseCertificateInDNS(*newEndorseCertificate);
}

void
ContactManager::onRefreshBrowseContact()
{
  return;

#if 0
  // The following no longer works as we don't serve such a list anymore
  vector<string> bufferedIdCertNames;
  try {
    using namespace boost::asio::ip;
    tcp::iostream request_stream;
    request_stream.expires_from_now(std::chrono::milliseconds(5000));
    request_stream.connect("ndncert.named-data.net","80");
    if (!request_stream) {
      emit warning(QString::fromStdString("Fail to fetch certificate directory! #1"));
      return;
    }

    request_stream << "GET /cert/list/ HTTP/1.0\r\n";
    request_stream << "Host: ndncert.named-data.net\r\n\r\n";
    request_stream.flush();

    string line1;
    std::getline(request_stream,line1);
    if (!request_stream) {
      emit warning(QString::fromStdString("Fail to fetch certificate directory! #2"));
      return;
    }

    std::stringstream response_stream(line1);
    string http_version;
    response_stream >> http_version;
    size_t status_code;
    response_stream >> status_code;
    string status_message;
    std::getline(response_stream,status_message);

    if (!response_stream ||
        http_version.substr(0,5) != "HTTP/") {
      emit warning(QString::fromStdString("Fail to fetch certificate directory! #3"));
      return;
    }
    if (status_code!=200) {
      emit warning(QString::fromStdString("Fail to fetch certificate directory! #4"));
      return;
    }
    vector<string> headers;
    string header;
    while (std::getline(request_stream, header) && header != "\r")
      headers.push_back(header);

    std::istreambuf_iterator<char> stream_iter (request_stream);
    std::istreambuf_iterator<char> end_of_stream;

    typedef boost::tokenizer< boost::escaped_list_separator<char>,
                              std::istreambuf_iterator<char> > tokenizer_t;
    tokenizer_t certItems (stream_iter,
                           end_of_stream,
                           boost::escaped_list_separator<char>('\\', '\n', '"'));

    for (tokenizer_t::iterator it = certItems.begin(); it != certItems.end (); it++)
      if (!it->empty())
        bufferedIdCertNames.push_back(*it);
  }
  catch (const std::exception& e) {
    emit warning(QString::fromStdString("Fail to fetch certificate directory! #N"));
  }

  {
    boost::recursive_mutex::scoped_lock lock(m_idCertCountMutex);
    m_idCertCount = bufferedIdCertNames.size();
  }
  m_bufferedIdCerts.clear();

  for (auto it = bufferedIdCertNames.begin(); it != bufferedIdCertNames.end(); it++) {
    Name certName(*it);

    Interest interest(certName);
    interest.setInterestLifetime(time::milliseconds(1000));
    interest.setMustBeFresh(true);
    interest.setCanBePrefix(true);

    ndn::security::DataValidationSuccessCallback onValidated =
    bind(&ContactManager::onIdentityCertValidated, this, _1);
    ndn::security::DataValidationFailureCallback onValidationFailed =
    bind(&ContactManager::onIdentityCertValidationFailed, this, _1, _2);
    TimeoutNotify timeoutNotify =
    bind(&ContactManager::onIdentityCertTimeoutNotify, this, _1);

    sendInterest(interest, onValidated, onValidationFailed, timeoutNotify, 0);
  }
#endif
}

void
ContactManager::onFetchIdCert(const QString& qCertName)
{
  Name certName(qCertName.toStdString());
  if (m_bufferedIdCerts.find(certName) != m_bufferedIdCerts.end())
    emit idCertReady(*m_bufferedIdCerts[certName]);
}

void
ContactManager::onAddFetchedContactIdCert(const QString& qCertName)
{
  Name certName(qCertName.toStdString());
  Name identity = certName.getPrefix(-1);

  auto it = m_bufferedIdCerts.find(certName);
  if (it != m_bufferedIdCerts.end()) {
    Contact contact(*it->second);
    try {
      m_contactStorage->addContact(contact);
      m_bufferedIdCerts.erase(certName);

      m_contactList.clear();
      m_contactStorage->getAllContacts(m_contactList);

      onWaitForContactList();
    }
    catch (const ContactStorage::Error& e) {
      emit warning(QString::fromStdString(e.what()));
    }
  }
  else
    emit warning(QString("Failure: no information of %1")
                 .arg(QString::fromStdString(identity.toUri())));
}

void
ContactManager::onWaitForContactList()
{
  QStringList aliasList;
  QStringList idList;
  for (auto it = m_contactList.begin(); it != m_contactList.end(); it++) {
    aliasList << QString((*it)->getAlias().c_str());
    idList << QString((*it)->getNameSpace().toUri().c_str());
  }

  emit contactAliasListReady(aliasList);
  emit contactIdListReady(idList);
}

void
ContactManager::onWaitForContactInfo(const QString& identity)
{
  for (auto it = m_contactList.begin(); it != m_contactList.end(); it++)
    if ((*it)->getNameSpace().toUri() == identity.toStdString())
      emit contactInfoReady(QString((*it)->getNameSpace().toUri().c_str()),
                            QString((*it)->getName().c_str()),
                            QString((*it)->getInstitution().c_str()),
                            (*it)->isIntroducer());
}

void
ContactManager::onRemoveContact(const QString& identity)
{
  m_contactStorage->removeContact(Name(identity.toStdString()));
  m_contactList.clear();
  m_contactStorage->getAllContacts(m_contactList);

  onWaitForContactList();
}

void
ContactManager::onUpdateAlias(const QString& identity, const QString& alias)
{
  m_contactStorage->updateAlias(Name(identity.toStdString()), alias.toStdString());
  m_contactList.clear();
  m_contactStorage->getAllContacts(m_contactList);

  onWaitForContactList();
}

void
ContactManager::onUpdateIsIntroducer(const QString& identity, bool isIntroducer)
{
  m_contactStorage->updateIsIntroducer(Name(identity.toStdString()), isIntroducer);
}

void
ContactManager::onUpdateEndorseCertificate(const QString& identity)
{
  Name identityName(identity.toStdString());
  shared_ptr<Certificate> newEndorseCertificate = generateEndorseCertificate(identityName);

  if (!static_cast<bool>(newEndorseCertificate))
    return;

  m_contactStorage->addEndorseCertificate(*newEndorseCertificate, identityName);

  publishEndorseCertificateInDNS(*newEndorseCertificate);
}

} // namespace chronochat


#if WAF
#include "contact-manager.moc"
#endif
