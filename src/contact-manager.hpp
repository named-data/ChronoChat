/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 *         Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_CONTACT_MANAGER_HPP
#define CHRONOCHAT_CONTACT_MANAGER_HPP

#include <QObject>

#ifndef Q_MOC_RUN
#include "common.hpp"
#include "contact-storage.hpp"
#include "endorse-certificate.hpp"
#include "profile.hpp"
#include "endorse-info.hpp"
#include "endorse-collection.hpp"
#include <ndn-cxx/security/key-chain.hpp>
#include <ndn-cxx/security/validator-config.hpp>
#include <ndn-cxx/face.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/recursive_mutex.hpp>
#endif

namespace chronochat {

typedef function<void(const Interest&)> TimeoutNotify;
typedef std::vector<shared_ptr<Contact> > ContactList;

class ContactManager : public QObject
{
  Q_OBJECT

public:
  ContactManager(ndn::Face& m_face, QObject* parent = 0);

  ~ContactManager();

  shared_ptr<Contact>
  getContact(const Name& identity)
  {
    return m_contactStorage->getContact(identity);
  }

  void
  getContactList(ContactList& contactList)
  {
    contactList.clear();
    contactList.insert(contactList.end(), m_contactList.begin(), m_contactList.end());
  }
private:
  void
  initializeSecurity();

  void
  fetchCollectEndorse(const Name& identity);

  void
  fetchEndorseCertificateInternal(const Name& identity, size_t certIndex);

  void
  prepareEndorseInfo(const Name& identity);

  // PROFILE: self-endorse-certificate
  void
  onDnsSelfEndorseCertValidated(const Data& selfEndorseCertificate,
                                const Name& identity);

  void
  onDnsSelfEndorseCertValidationFailed(const Data& selfEndorseCertificate,
                                       const ndn::security::ValidationError& error,
                                       const Name& identity);

  void
  onDnsSelfEndorseCertTimeoutNotify(const Interest& interest, const Name& identity);

  // ENDORSED: endorse-collection
  void
  onDnsCollectEndorseValidated(const Data& data, const Name& identity);

  void
  onDnsCollectEndorseValidationFailed(const Data& data,
                                      const ndn::security::ValidationError& error,
                                      const Name& identity);

  void
  onDnsCollectEndorseTimeoutNotify(const Interest& interest,
                                   const Name& identity);

  // PROFILE-CERT: endorse-certificate
  void
  onEndorseCertificateInternal(const Interest& interest, const Data& data,
                               const Name& identity, size_t certIndex,
                               std::string hash);

  void
  onEndorseCertificateInternalTimeout(const Interest& interest,
                                      const Name& identity,
                                      size_t certIndex);

  // Collect endorsement
  void
  collectEndorsement();

  void
  onDnsEndorseeValidated(const Data& data);

  void
  onDnsEndorseeValidationFailed(const Data& data,
                                const ndn::security::ValidationError& error);

  void
  onDnsEndorseeTimeoutNotify(const Interest& interest);

  void
  decreaseCollectStatus();

  void
  publishCollectEndorsedDataInDNS();

  // Identity certificate
  void
  onIdentityCertValidated(const Data& data);

  void
  onIdentityCertValidationFailed(const Data& data,
                                 const ndn::security::ValidationError& error);

  void
  onIdentityCertTimeoutNotify(const Interest& interest);

  void
  decreaseIdCertCount();

  // Publish self-endorse certificate
  shared_ptr<EndorseCertificate>
  getSignedSelfEndorseCertificate(const Profile& profile);

  void
  publishSelfEndorseCertificateInDNS(const EndorseCertificate& selfEndorseCertificate);

  // Publish endorse certificate
  shared_ptr<EndorseCertificate>
  generateEndorseCertificate(const Name& identity);

  void
  publishEndorseCertificateInDNS(const EndorseCertificate& endorseCertificate);

  // Communication
  void
  sendInterest(const Interest& interest,
               const ndn::security::DataValidationSuccessCallback& onValidated,
               const ndn::security::DataValidationFailureCallback& onValidationFailed,
               const TimeoutNotify& timeoutNotify,
               int retry = 1);

  void
  onTargetData(const Interest& interest,
               const Data& data,
               const ndn::security::DataValidationSuccessCallback& onValidated,
               const ndn::security::DataValidationFailureCallback& onValidationFailed);

  void
  onTargetTimeout(const Interest& interest,
                  int retry,
                  const ndn::security::DataValidationSuccessCallback& onValidated,
                  const ndn::security::DataValidationFailureCallback& onValidationFailed,
                  const TimeoutNotify& timeoutNotify);

  // DNS listener
  void
  onDnsInterest(const Name& prefix, const Interest& interest);

  void
  onDnsRegisterFailed(const Name& prefix, const std::string& failInfo);

  void
  onKeyInterest(const Name& prefix, const Interest& interest);

signals:
  void
  contactEndorseInfoReady(const EndorseInfo& endorseInfo);

  void
  contactInfoFetchFailed(const QString& identity);

  void
  idCertNameListReady(const QStringList& certNameList);

  void
  nameListReady(const QStringList& certNameList);

  void
  idCertReady(const ndn::security::Certificate& idCert);

  void
  contactAliasListReady(const QStringList& aliasList);

  void
  contactIdListReady(const QStringList& idList);

  void
  contactInfoReady(const QString& identity,
                   const QString& name,
                   const QString& institute,
                   bool isIntro);

  void
  warning(const QString& msg);

public slots:
  void
  onIdentityUpdated(const QString& identity);

  void
  onFetchContactInfo(const QString& identity);

  void
  onAddFetchedContact(const QString& identity);

  void
  onUpdateProfile();

  void
  onRefreshBrowseContact();

  void
  onFetchIdCert(const QString& certName);

  void
  onAddFetchedContactIdCert(const QString& identity);

  void
  onWaitForContactList();

  void
  onWaitForContactInfo(const QString& identity);

  void
  onRemoveContact(const QString& identity);

  void
  onUpdateAlias(const QString& identity, const QString& alias);

  void
  onUpdateIsIntroducer(const QString& identity, bool isIntro);

  void
  onUpdateEndorseCertificate(const QString& identity);

private:

  class FetchedInfo {
  public:
    shared_ptr<EndorseCertificate> m_selfEndorseCert;
    shared_ptr<EndorseCollection> m_endorseCollection;
    std::vector<shared_ptr<EndorseCertificate> > m_endorseCertList;
    shared_ptr<EndorseInfo> m_endorseInfo;
  };

  typedef std::map<Name, FetchedInfo> BufferedContacts;
  typedef std::map<Name, shared_ptr<ndn::security::Certificate> > BufferedIdCerts;

  typedef boost::recursive_mutex RecLock;
  typedef boost::unique_lock<RecLock> UniqueRecLock;

  // Conf
  shared_ptr<ContactStorage> m_contactStorage;
  shared_ptr<ndn::security::ValidatorConfig> m_validator;
  ndn::Face& m_face;
  ndn::KeyChain m_keyChain;
  Name m_identity;
  ContactList m_contactList;

  // Buffer
  BufferedContacts m_bufferedContacts;
  BufferedIdCerts m_bufferedIdCerts;

  // Tmp Dns
  shared_ptr<ndn::RegisteredPrefixHandle> m_dnsListenerId;
  shared_ptr<ndn::RegisteredPrefixHandle> m_keyListenerId;
  shared_ptr<ndn::RegisteredPrefixHandle> m_profileCertListenerId;

  RecLock m_collectCountMutex;
  size_t m_collectCount;

  RecLock m_idCertCountMutex;
  size_t m_idCertCount;
};

} // namespace chronochat

#endif // CHRONOCHAT_CONTACT_MANAGER_HPP
