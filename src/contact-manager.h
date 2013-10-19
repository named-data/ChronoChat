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
#include <ndn.cxx/wrapper/wrapper.h>
#endif


class ContactManager : public QObject
{
  Q_OBJECT

public:
  ContactManager(ndn::Ptr<ContactStorage> contactStorage,
                 ndn::Ptr<DnsStorage> dnsStorage,
                 QObject* parent = 0);

  ~ContactManager();

  void
  fetchSelfEndorseCertificate(const ndn::Name& identity);

  void
  updateProfileData(const ndn::Name& identity);

  inline ndn::Ptr<ContactStorage>
  getContactStorage()
  { return m_contactStorage; }

  inline ndn::Ptr<DnsStorage>
  getDnsStorage()
  { return m_dnsStorage; }

  inline ndn::Name
  getDefaultIdentity()
  { return m_keychain->getDefaultIdentity(); }

  inline ndn::Ptr<ndn::Wrapper>
  getWrapper()
  { return m_wrapper; }

private:
  void
  setKeychain();

  ndn::Ptr<EndorseCertificate>
  getSignedSelfEndorseCertificate(const ndn::Name& identity, const Profile& profile);

  void
  publishSelfEndorseCertificateInDNS(ndn::Ptr<EndorseCertificate> selfEndorseCertificate);

  void 
  onDnsSelfEndorseCertificateVerified(ndn::Ptr<ndn::Data> selfEndorseCertificate, const ndn::Name& identity);

  void
  onDnsSelfEndorseCertificateUnverified(ndn::Ptr<ndn::Data> selfEndorseCertificate, const ndn::Name& identity);

  void
  onDnsSelfEndorseCertificateTimeout(ndn::Ptr<ndn::Closure> closure, ndn::Ptr<ndn::Interest> interest, const ndn::Name& identity, int retry);

signals:
  void 
  contactFetched(ndn::Ptr<EndorseCertificate> selfEndorseCertificate);
  
  void
  contactFetchFailed(const ndn::Name& identity);

private slots:
  
  
private:
  ndn::Ptr<ContactStorage> m_contactStorage;
  ndn::Ptr<DnsStorage> m_dnsStorage;
  ndn::Ptr<ndn::security::Keychain> m_keychain;
  ndn::Ptr<ndn::Wrapper> m_wrapper;
};

#endif
