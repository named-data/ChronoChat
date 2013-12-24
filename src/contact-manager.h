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
#include <ndn-cpp/face.hpp>
#include <ndn-cpp/security/policy/validation-request.hpp>
#include <ndn-cpp-et/policy-manager/simple-policy-manager.hpp>
#endif

typedef ndn::func_lib::function<void()> TimeoutNotify;

class ContactManager : public QObject
{
  Q_OBJECT

public:
  ContactManager(ndn::ptr_lib::shared_ptr<ndn::IdentityManager> identityManager,
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

  void
  getContactItemList(std::vector<ndn::ptr_lib::shared_ptr<ContactItem> >& contacts);

  ndn::ptr_lib::shared_ptr<ContactStorage>
  getContactStorage()
  { return m_contactStorage; }

  ndn::ptr_lib::shared_ptr<ContactItem>
  getContact(const ndn::Name& contactNamespace);

  ndn::ptr_lib::shared_ptr<DnsStorage>
  getDnsStorage()
  { return m_dnsStorage; }

  ndn::Name
  getDefaultIdentity()
  { return m_identityManager->getDefaultIdentity(); }

  void
  publishEndorsedDataInDns(const ndn::Name& identity);

  void
  setDefaultIdentity(const ndn::Name& identity)
  { m_defaultIdentity = identity; }

  void
  addContact(const ndn::IdentityCertificate& idCert, const Profile& profile);

  void
  removeContact(const ndn::Name& contactNameSpace);
  
  ndn::ptr_lib::shared_ptr<ndn::IdentityManager>
  getIdentityManager()
  { return m_identityManager; }

private:
  void 
  connectToDaemon();

  void
  onConnectionData(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest,
                   const ndn::ptr_lib::shared_ptr<ndn::Data>& data);
 
  void
  onConnectionDataTimeout(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest);
  
  void
  initializeSecurity();

  ndn::ptr_lib::shared_ptr<EndorseCertificate>
  getSignedSelfEndorseCertificate(const ndn::Name& identity, const Profile& profile);

  ndn::ptr_lib::shared_ptr<EndorseCertificate> 
  generateEndorseCertificate(const ndn::Name& identity, const ndn::Name& signerIdentity);

  void
  publishSelfEndorseCertificateInDNS(const EndorseCertificate& selfEndorseCertificate);

  void
  publishEndorseCertificateInDNS(const EndorseCertificate& endorseCertificate, const ndn::Name& signerIdentity);

  void
  sendInterest(const ndn::Interest& interest,
               const ndn::OnVerified& onVerified,
               const ndn::OnVerifyFailed& onVerifyFailed,
               const TimeoutNotify& timeoutNotify,
               int retry = 1,
               int stepCount = 0);

  void
  onTargetData(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest, 
               const ndn::ptr_lib::shared_ptr<ndn::Data>& data,
               int stepCount,
               const ndn::OnVerified& onVerified,
               const ndn::OnVerifyFailed& onVerifyFailed,
               const TimeoutNotify& timeoutNotify);

  void
  onTargetTimeout(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest, 
                  int retry,
                  int stepCount,
                  const ndn::OnVerified& onVerified,
                  const ndn::OnVerifyFailed& onVerifyFailed,
                  const TimeoutNotify& timeoutNotify);


  void
  onCertData(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest, 
             const ndn::ptr_lib::shared_ptr<ndn::Data>& cert,
             ndn::ptr_lib::shared_ptr<ndn::ValidationRequest> previousStep);

  void
  onCertTimeout(const ndn::ptr_lib::shared_ptr<const ndn::Interest>& interest,
                const ndn::OnVerifyFailed& onVerifyFailed,
                const ndn::ptr_lib::shared_ptr<ndn::Data>& data,
                ndn::ptr_lib::shared_ptr<ndn::ValidationRequest> nextStep);


  void
  onDnsSelfEndorseCertificateTimeoutNotify(const ndn::Name& identity);

  void 
  onDnsSelfEndorseCertificateVerified(const ndn::ptr_lib::shared_ptr<ndn::Data>& selfEndorseCertificate, const ndn::Name& identity);

  void
  onDnsSelfEndorseCertificateVerifyFailed(const ndn::ptr_lib::shared_ptr<ndn::Data>& selfEndorseCertificate, const ndn::Name& identity);
 

  void
  onDnsCollectEndorseVerified(const ndn::ptr_lib::shared_ptr<ndn::Data>& data, const ndn::Name& identity);

  void
  onDnsCollectEndorseVerifyFailed(const ndn::ptr_lib::shared_ptr<ndn::Data>& data, const ndn::Name& identity);

  void
  onDnsCollectEndorseTimeoutNotify(const ndn::Name& identity);


  void
  onKeyVerified(const ndn::ptr_lib::shared_ptr<ndn::Data>& data, const ndn::Name& identity);

  void
  onKeyVerifyFailed(const ndn::ptr_lib::shared_ptr<ndn::Data>& data, const ndn::Name& identity);

  void
  onKeyTimeoutNotify(const ndn::Name& identity);


  void
  onIdCertificateVerified(const ndn::ptr_lib::shared_ptr<ndn::Data>& data, const ndn::Name& identity);

  void
  onIdCertificateVerifyFailed(const ndn::ptr_lib::shared_ptr<ndn::Data>& data, const ndn::Name& identity);

  void
  onIdCertificateTimeoutNotify(const ndn::Name& identity);
  

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
  ndn::ptr_lib::shared_ptr<ContactStorage> m_contactStorage;
  ndn::ptr_lib::shared_ptr<DnsStorage> m_dnsStorage;
  ndn::ptr_lib::shared_ptr<ndn::SimplePolicyManager> m_policyManager;
  ndn::ptr_lib::shared_ptr<ndn::IdentityManager> m_identityManager;
  ndn::ptr_lib::shared_ptr<ndn::Face> m_face;
  ndn::ptr_lib::shared_ptr<ndn::Transport> m_transport;
  ndn::Name m_defaultIdentity;
};

#endif
