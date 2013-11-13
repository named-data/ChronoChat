/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef LINKNDN_CONTACT_MANAGER_H
#define LINKNDN_CONTACT_MANAGER_H

#include <QObject>

#ifndef Q_MOC_RUN
#include "contact-storage.h"
#include "dns-storage.h"
#include "endorse-certificate.h"
#include "profile.h"
#include <ndn.cxx/wrapper/wrapper.h>
#endif


class ContactManager : public QObject
{
  Q_OBJECT

public:
  ContactManager(QObject* parent = 0);

  ~ContactManager();

  void
  setWrapper();

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

  std::vector<ndn::Ptr<ContactItem> >
  getContactItemList();

  inline ndn::Ptr<ContactStorage>
  getContactStorage()
  { return m_contactStorage; }

  ndn::Ptr<ContactItem>
  getContact(const ndn::Name& contactNamespace);

  inline ndn::Ptr<DnsStorage>
  getDnsStorage()
  { return m_dnsStorage; }

  inline ndn::Name
  getDefaultIdentity()
  { return m_keychain->getDefaultIdentity(); }

  inline ndn::Ptr<ndn::Wrapper>
  getWrapper()
  { return m_wrapper; }

  void
  publishEndorsedDataInDns(const ndn::Name& identity);

  inline void
  setDefaultIdentity(const ndn::Name& identity)
  { m_defaultIdentity = identity; }

  void
  addContact(const ndn::security::IdentityCertificate& idCert, const Profile& profile);

  void
  removeContact(const ndn::Name& contactNameSpace);
  

private:
  void
  setKeychain();

  ndn::Ptr<EndorseCertificate>
  getSignedSelfEndorseCertificate(const ndn::Name& identity, const Profile& profile);

  ndn::Ptr<EndorseCertificate> 
  generateEndorseCertificate(const ndn::Name& identity, const ndn::Name& signerIdentity);

  void
  publishSelfEndorseCertificateInDNS(ndn::Ptr<EndorseCertificate> selfEndorseCertificate);

  void
  publishEndorseCertificateInDNS(ndn::Ptr<EndorseCertificate> endorseCertificate, const ndn::Name& signerIdentity);

  void 
  onDnsSelfEndorseCertificateVerified(ndn::Ptr<ndn::Data> selfEndorseCertificate, const ndn::Name& identity);

  void
  onDnsSelfEndorseCertificateUnverified(ndn::Ptr<ndn::Data> selfEndorseCertificate, const ndn::Name& identity);

  void
  onDnsSelfEndorseCertificateTimeout(ndn::Ptr<ndn::Closure> closure, ndn::Ptr<ndn::Interest> interest, const ndn::Name& identity, int retry);

  void
  onKeyVerified(ndn::Ptr<ndn::Data> data, const ndn::Name& identity);

  void
  onKeyUnverified(ndn::Ptr<ndn::Data> data, const ndn::Name& identity);

  void
  onKeyTimeout(ndn::Ptr<ndn::Closure> closure, ndn::Ptr<ndn::Interest> interest, const ndn::Name& identity, int retry);

  void
  onDnsCollectEndorseVerified(ndn::Ptr<ndn::Data> data, const ndn::Name& identity);

  void
  onDnsCollectEndorseTimeout(ndn::Ptr<ndn::Closure> closure, ndn::Ptr<ndn::Interest> interest, const ndn::Name& identity, int retry);

  void
  onDnsCollectEndorseUnverified(ndn::Ptr<ndn::Data> data, const ndn::Name& identity);

  void
  onIdCertificateVerified(ndn::Ptr<ndn::Data> data, const ndn::Name& identity);

  void
  onIdCertificateUnverified(ndn::Ptr<ndn::Data> data, const ndn::Name& identity);

  void
  onIdCertificateTimeout(ndn::Ptr<ndn::Closure> closure, ndn::Ptr<ndn::Interest> interest, const ndn::Name& identity, int retry);
  

signals:
  void
  noNdnConnection(const QString& msg);
  
  void 
  contactFetched(const EndorseCertificate& endorseCertificate);
  
  void
  contactFetchFailed(const ndn::Name& identity);

  void
  contactKeyFetched(const EndorseCertificate& endorseCertificate);

  void
  contactKeyFetchFailed(const ndn::Name& identity);

  void 
  contactCertificateFetched(const ndn::security::IdentityCertificate& identityCertificate);
  
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
  ndn::Ptr<ContactStorage> m_contactStorage;
  ndn::Ptr<DnsStorage> m_dnsStorage;
  ndn::Ptr<ndn::security::Keychain> m_keychain;
  ndn::Ptr<ndn::Wrapper> m_wrapper;
  ndn::Name m_defaultIdentity;
};

#endif
