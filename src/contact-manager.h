/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOS_CONTACT_MANAGER_H
#define CHRONOS_CONTACT_MANAGER_H

#include <QObject>

#ifndef Q_MOC_RUN
#include "contact-storage.h"
#include "endorse-certificate.h"
#include "profile.h"
#include "endorse-info.pb.h"
#include "endorse-collection.pb.h"
#include <ndn-cpp-dev/face.hpp>
#include <ndn-cpp-dev/security/key-chain.hpp>
#include <ndn-cpp-dev/security/validator.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/recursive_mutex.hpp>
#endif

namespace chronos{

typedef ndn::function<void(const ndn::Interest&)> TimeoutNotify;
typedef std::vector<ndn::shared_ptr<Contact> > ContactList;

class ContactManager : public QObject
{
  Q_OBJECT

public:
  ContactManager(ndn::shared_ptr<ndn::Face> m_face,
                 QObject* parent = 0);

  ~ContactManager();

  ndn::shared_ptr<Contact>
  getContact(const ndn::Name& identity)
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
  fetchCollectEndorse(const ndn::Name& identity);

  void
  fetchEndorseCertificateInternal(const ndn::Name& identity, int certIndex);

  void
  prepareEndorseInfo(const ndn::Name& identity);

  // PROFILE: self-endorse-certificate
  void 
  onDnsSelfEndorseCertValidated(const ndn::shared_ptr<const ndn::Data>& selfEndorseCertificate, 
                                const ndn::Name& identity);

  void
  onDnsSelfEndorseCertValidationFailed(const ndn::shared_ptr<const ndn::Data>& selfEndorseCertificate, 
                                       const ndn::Name& identity);

  void
  onDnsSelfEndorseCertTimeoutNotify(const ndn::Interest& interest,
                                    const ndn::Name& identity);
 
  // ENDORSED: endorse-collection
  void
  onDnsCollectEndorseValidated(const ndn::shared_ptr<const ndn::Data>& data, 
                               const ndn::Name& identity);

  void
  onDnsCollectEndorseValidationFailed(const ndn::shared_ptr<const ndn::Data>& data, 
                                      const ndn::Name& identity);

  void
  onDnsCollectEndorseTimeoutNotify(const ndn::Interest& interest,
                                   const ndn::Name& identity);  

  // PROFILE-CERT: endorse-certificate
  void
  onEndorseCertificateInternal(const ndn::Interest& interest,
                               ndn::Data& data, 
                               const ndn::Name& identity, 
                               int certIndex,
                               std::string hash);

  void
  onEndorseCertificateInternalTimeout(const ndn::Interest& interest,
                                      const ndn::Name& identity, 
                                      int certIndex);

  // Collect endorsement
  void
  collectEndorsement();

  void
  onDnsEndorseeValidated(const ndn::shared_ptr<const ndn::Data>& data);

  void
  onDnsEndorseeValidationFailed(const ndn::shared_ptr<const ndn::Data>& data);

  void
  onDnsEndorseeTimeoutNotify(const ndn::Interest& interest);

  void
  decreaseCollectStatus();

  void
  publishCollectEndorsedDataInDNS();

  // Identity certificate
  void
  onIdentityCertValidated(const ndn::shared_ptr<const ndn::Data>& data);
  
  void
  onIdentityCertValidationFailed(const ndn::shared_ptr<const ndn::Data>& data);

  void
  onIdentityCertTimeoutNotify(const ndn::Interest& interest);

  void
  decreaseIdCertCount();

  // Publish self-endorse certificate
  ndn::shared_ptr<EndorseCertificate>
  getSignedSelfEndorseCertificate(const Profile& profile);

  void
  publishSelfEndorseCertificateInDNS(const EndorseCertificate& selfEndorseCertificate);

  // Publish endorse certificate
  ndn::shared_ptr<EndorseCertificate> 
  generateEndorseCertificate(const ndn::Name& identity);

  void
  publishEndorseCertificateInDNS(const EndorseCertificate& endorseCertificate);

  // Communication
  void
  sendInterest(const ndn::Interest& interest,
               const ndn::OnDataValidated& onValidated,
               const ndn::OnDataValidationFailed& onValidationFailed,
               const TimeoutNotify& timeoutNotify,
               int retry = 1);

  void
  onTargetData(const ndn::Interest& interest, 
               const ndn::Data& data,
               const ndn::OnDataValidated& onValidated,
               const ndn::OnDataValidationFailed& onValidationFailed);

  void
  onTargetTimeout(const ndn::Interest& interest, 
                  int retry,
                  const ndn::OnDataValidated& onValidated,
                  const ndn::OnDataValidationFailed& onValidationFailed,
                  const TimeoutNotify& timeoutNotify);

  // DNS listener
  void
  onDnsInterest(const ndn::Name& prefix, const ndn::Interest& interest);
  
  void
  onDnsRegisterFailed(const ndn::Name& prefix, const std::string& failInfo);

signals:
  void
  contactEndorseInfoReady(const chronos::EndorseInfo& endorseInfo);

  void
  contactInfoFetchFailed(const QString& identity);

  void
  idCertNameListReady(const QStringList& certNameList);
  
  void
  nameListReady(const QStringList& certNameList);

  void
  idCertReady(const ndn::IdentityCertificate& idCert);

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
    ndn::shared_ptr<EndorseCertificate> m_selfEndorseCert;
    ndn::shared_ptr<EndorseCollection> m_endorseCollection;
    std::vector<ndn::shared_ptr<EndorseCertificate> > m_endorseCertList;
    ndn::shared_ptr<EndorseInfo> m_endorseInfo;
  };

  typedef std::map<ndn::Name, FetchedInfo> BufferedContacts;
  typedef std::map<ndn::Name, ndn::shared_ptr<ndn::IdentityCertificate> > BufferedIdCerts;

  typedef boost::recursive_mutex RecLock;
  typedef boost::unique_lock<RecLock> UniqueRecLock;

  // Conf
  ndn::shared_ptr<ContactStorage> m_contactStorage;
  ndn::shared_ptr<ndn::Validator> m_validator;
  ndn::shared_ptr<ndn::Face> m_face;
  ndn::KeyChain m_keyChain;
  ndn::Name m_identity;
  ContactList m_contactList;

  // Buffer
  BufferedContacts m_bufferedContacts;
  BufferedIdCerts m_bufferedIdCerts;

  // Tmp Dns
  const ndn::RegisteredPrefixId* m_dnsListenerId;

  RecLock m_collectCountMutex;
  int m_collectCount;

  RecLock m_idCertCountMutex;
  int m_idCertCount;
};

} // namespace chronos

#endif //CHRONOS_CONTACT_MANAGER_H
