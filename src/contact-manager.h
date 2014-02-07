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
#include "dns-storage.h"
#include "endorse-certificate.h"
#include "profile.h"
#include <ndn-cpp-dev/face.hpp>
#include <ndn-cpp-dev/security/key-chain.hpp>
#include <ndn-cpp-dev/security/validator.hpp>
#endif

namespace chronos{

typedef ndn::function<void()> TimeoutNotify;

class ContactManager : public QObject
{
  Q_OBJECT

public:
  ContactManager(ndn::shared_ptr<ndn::Face> m_face,
                 QObject* parent = 0);

  ~ContactManager();

  void
  fetchSelfEndorseCertificate(const ndn::Name& identity);

  void
  fetchKey(const ndn::Name& identity);

  void
  fetchCollectEndorse(const ndn::Name& identity);

  void
  fetchIdCertificate(const ndn::Name& certName);

  void
  updateProfileData(const ndn::Name& identity);

  void
  updateEndorseCertificate(const ndn::Name& identity, const ndn::Name& signerIdentity);

  inline void
  getContactItemList(std::vector<ndn::shared_ptr<ContactItem> >& contacts);

  ndn::shared_ptr<ContactStorage>
  getContactStorage()
  { return m_contactStorage; }

  inline ndn::shared_ptr<ContactItem>
  getContact(const ndn::Name& contactNamespace);

  ndn::shared_ptr<DnsStorage>
  getDnsStorage()
  { return m_dnsStorage; }

  ndn::Name
  getDefaultIdentity()
  { return m_keyChain->getDefaultIdentity(); }

  void
  publishCollectEndorsedDataInDNS(const ndn::Name& identity);

  void
  setDefaultIdentity(const ndn::Name& identity)
  { m_defaultIdentity = identity; }

  void
  addContact(const ndn::IdentityCertificate& idCert, const Profile& profile);

  void
  removeContact(const ndn::Name& contactNameSpace);
  
private:  
  void
  initializeSecurity();

  ndn::shared_ptr<EndorseCertificate>
  getSignedSelfEndorseCertificate(const ndn::Name& identity, const Profile& profile);

  ndn::shared_ptr<EndorseCertificate> 
  generateEndorseCertificate(const ndn::Name& identity, const ndn::Name& signerIdentity);

  void
  publishSelfEndorseCertificateInDNS(const EndorseCertificate& selfEndorseCertificate);

  void
  publishEndorseCertificateInDNS(const EndorseCertificate& endorseCertificate, const ndn::Name& signerIdentity);

  inline void
  sendInterest(const ndn::Interest& interest,
               const ndn::OnDataValidated& onValidated,
               const ndn::OnDataValidationFailed& onValidationFailed,
               const TimeoutNotify& timeoutNotify,
               int retry = 1);

  inline void
  onTargetData(const ndn::Interest& interest, 
               const ndn::Data& data,
               const ndn::OnDataValidated& onValidated,
               const ndn::OnDataValidationFailed& onValidationFailed);

  inline void
  onTargetTimeout(const ndn::Interest& interest, 
                  int retry,
                  const ndn::OnDataValidated& onValidated,
                  const ndn::OnDataValidationFailed& onValidationFailed,
                  const TimeoutNotify& timeoutNotify);

  void 
  onDnsSelfEndorseCertValidated(const ndn::shared_ptr<const ndn::Data>& selfEndorseCertificate, const ndn::Name& identity);

  inline void
  onDnsSelfEndorseCertValidationFailed(const ndn::shared_ptr<const ndn::Data>& selfEndorseCertificate, const ndn::Name& identity);

  inline void
  onDnsSelfEndorseCertTimeoutNotify(const ndn::Name& identity);
 

  inline void
  onDnsCollectEndorseValidated(const ndn::shared_ptr<const ndn::Data>& data, const ndn::Name& identity);

  inline void
  onDnsCollectEndorseValidationFailed(const ndn::shared_ptr<const ndn::Data>& data, const ndn::Name& identity);

  inline void
  onDnsCollectEndorseTimeoutNotify(const ndn::Name& identity);


  void
  onKeyValidated(const ndn::shared_ptr<const ndn::Data>& data, const ndn::Name& identity);

  inline void
  onKeyValidationFailed(const ndn::shared_ptr<const ndn::Data>& data, const ndn::Name& identity);

  inline void
  onKeyTimeoutNotify(const ndn::Name& identity);


  inline void
  onIdCertValidated(const ndn::shared_ptr<const ndn::Data>& data, const ndn::Name& identity);

  inline void
  onIdCertValidationFailed(const ndn::shared_ptr<const ndn::Data>& data, const ndn::Name& identity);

  inline void
  onIdCertTimeoutNotify(const ndn::Name& identity);
  

signals:
  void
  noNdnConnection(const QString& msg);
  
  void 
  contactFetched(const chronos::EndorseCertificate& endorseCertificate);
  
  void
  contactFetchFailed(const ndn::Name& identity);

  void
  contactKeyFetched(const chronos::EndorseCertificate& endorseCertificate);

  void
  contactKeyFetchFailed(const ndn::Name& identity);

  void 
  contactCertificateFetched(const ndn::IdentityCertificate& identityCertificate);
  
  void
  contactCertificateFetchFailed(const ndn::Name& identity);

  void 
  collectEndorseFetched(const ndn::Data& data);

  void
  collectEndorseFetchFailed(const ndn::Name& identity);

  void
  warning(QString msg);

  void
  contactRemoved(const ndn::Name& identity);

  void
  contactAdded(const ndn::Name& identity);                                         

private slots:
  
  
private:

  ndn::shared_ptr<ContactStorage> m_contactStorage;
  ndn::shared_ptr<DnsStorage> m_dnsStorage;
  ndn::shared_ptr<ndn::Validator> m_validator;
  ndn::shared_ptr<ndn::Face> m_face;
  ndn::shared_ptr<ndn::KeyChain> m_keyChain;
  ndn::Name m_defaultIdentity;
};

void
ContactManager::sendInterest(const ndn::Interest& interest,
                             const ndn::OnDataValidated& onValidated,
                             const ndn::OnDataValidationFailed& onValidationFailed,
                             const TimeoutNotify& timeoutNotify,
                             int retry /* = 1 */)
{
  m_face->expressInterest(interest, 
                          bind(&ContactManager::onTargetData, 
                               this, _1, _2, onValidated, onValidationFailed),
                          bind(&ContactManager::onTargetTimeout,
                               this, _1, retry, onValidated, onValidationFailed, timeoutNotify));
}

void
ContactManager::onTargetData(const ndn::Interest& interest, 
                             const ndn::Data& data,
                             const ndn::OnDataValidated& onValidated,
                             const ndn::OnDataValidationFailed& onValidationFailed)
{ m_validator->validate(data, onValidated, onValidationFailed); }

void
ContactManager::onTargetTimeout(const ndn::Interest& interest, 
                                int retry,
                                const ndn::OnDataValidated& onValidated,
                                const ndn::OnDataValidationFailed& onValidationFailed,
                                const TimeoutNotify& timeoutNotify)
{
  if(retry > 0)
    sendInterest(interest, onValidated, onValidationFailed, timeoutNotify, retry-1);
  else
    timeoutNotify();
}


void
ContactManager::onDnsSelfEndorseCertValidationFailed(const ndn::shared_ptr<const ndn::Data>& data, 
                                                     const ndn::Name& identity)
{ emit contactFetchFailed (identity); }

void
ContactManager::onDnsSelfEndorseCertTimeoutNotify(const ndn::Name& identity)
{ emit contactFetchFailed(identity); }

void
ContactManager::onDnsCollectEndorseValidated(const ndn::shared_ptr<const ndn::Data>& data, 
                                            const ndn::Name& identity)
{ emit collectEndorseFetched (*data); }

void
ContactManager::onDnsCollectEndorseValidationFailed(const ndn::shared_ptr<const ndn::Data>& data, 
                                                const ndn::Name& identity)
{ emit collectEndorseFetchFailed (identity); }

void
ContactManager::onDnsCollectEndorseTimeoutNotify(const ndn::Name& identity)
{ emit collectEndorseFetchFailed (identity); }

void
ContactManager::onKeyValidationFailed(const ndn::shared_ptr<const ndn::Data>& data, 
                                  const ndn::Name& identity)
{ emit contactKeyFetchFailed (identity); }

void
ContactManager::onKeyTimeoutNotify(const ndn::Name& identity)
{ emit contactKeyFetchFailed(identity); }

void
ContactManager::onIdCertValidated(const ndn::shared_ptr<const ndn::Data>& data, 
                                        const ndn::Name& identity)
{
  ndn::IdentityCertificate identityCertificate(*data);
  emit contactCertificateFetched(identityCertificate);
}

void
ContactManager::onIdCertValidationFailed(const ndn::shared_ptr<const ndn::Data>& data, 
                                            const ndn::Name& identity)
{ emit contactCertificateFetchFailed (identity); }

void
ContactManager::onIdCertTimeoutNotify(const ndn::Name& identity)
{ emit contactCertificateFetchFailed (identity); }

ndn::shared_ptr<ContactItem>
ContactManager::getContact(const ndn::Name& contactNamespace)
{ return m_contactStorage->getContact(contactNamespace); }

void
ContactManager::getContactItemList(std::vector<ndn::shared_ptr<ContactItem> >& contacts)
{ return m_contactStorage->getAllContacts(contacts); }

}//chronos

#endif
