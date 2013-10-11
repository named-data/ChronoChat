/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef LINKEDN_ENDORSE_CERTIFICATE_H
#define LINKEDN_ENDORSE_CERTIFICATE_H

#include <vector>
#include <ndn.cxx/data.h>
#include <ndn.cxx/security/certificate/identity-certificate.h>
#include <ndn.cxx/security/certificate/certificate-extension.h>

#include "profile-data.h"

class EndorseExtension : public ndn::security::CertificateExtension
{
public:
  EndorseExtension(const ndn::Blob & value);

  virtual
  ~EndorseExtension() {}
};

class EndorseCertificate : public ndn::security::Certificate
{
public:
  EndorseCertificate(const ndn::security::IdentityCertificate& kskCertificate,
                     const ndn::Name& signer,
                     const ndn::Time& notBefore,
                     const ndn::Time& notAfter);

  EndorseCertificate(const EndorseCertificate& endorseCertificate);

  EndorseCertificate(const ndn::Data& data);

  virtual
  ~EndorseCertificate()
  {}

  void 
  addProfile(const ProfileData& profile);

  void
  encode();

  inline const ndn::Name&
  getSigner() const
  { return m_signer; }

  inline const std::vector<ProfileData>&
  getProfileList() const
  { return m_profileList; }

  inline ndn::Name
  getPublicKeyName ()
  { return m_keyName; }

private:
  void 
  decode();

private:
  ndn::Name m_keyName;
  ndn::Name m_signer;
  std::vector<ProfileData> m_profileList;
};

#endif
