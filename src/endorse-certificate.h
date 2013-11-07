/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef LINKNDN_ENDORSE_CERTIFICATE_H
#define LINKNDN_ENDORSE_CERTIFICATE_H

#include <vector>
#include <ndn.cxx/data.h>
#include <ndn.cxx/security/certificate/identity-certificate.h>
#include <ndn.cxx/security/certificate/certificate-extension.h>

#include "profile-data.h"

class ProfileExtension : public ndn::security::CertificateExtension
{
public:
  ProfileExtension(const ProfileData& profileData);
  
  ProfileExtension(const ProfileExtension& profileExtension);

  ProfileExtension(const CertificateExtension& extension);

  ~ProfileExtension() {}

  ndn::Ptr<ProfileData>
  getProfileData();
};

class EndorseExtension : public ndn::security::CertificateExtension
{
public:
  EndorseExtension(const std::vector<std::string>& endorsedList);

  EndorseExtension(const EndorseExtension& endorseExtension);

  EndorseExtension(const CertificateExtension& extension);

  ~EndorseExtension() {}

  std::vector<std::string>
  getEndorsedList();

private:
  static ndn::Ptr<ndn::Blob>
  prepareValue(const std::vector<std::string>& endorsedList);
};

class EndorseCertificate : public ndn::security::Certificate
{
public:
  EndorseCertificate() {}

  EndorseCertificate(const ndn::security::IdentityCertificate& kskCertificate,
                     ndn::Ptr<ProfileData> profileData,
                     const std::vector<std::string>& endorseList = std::vector<std::string>());

  EndorseCertificate(const EndorseCertificate& endorseCertificate,
                     const ndn::Name& signer,
                     const std::vector<std::string>& endorseList);

  EndorseCertificate(const EndorseCertificate& endorseCertificate);

  EndorseCertificate(const ndn::Data& data);

  virtual
  ~EndorseCertificate()
  {}

  inline const ndn::Name&
  getSigner() const
  { return m_signer; }

  inline ndn::Ptr<ProfileData>
  getProfileData() const
  { return m_profileData; }

  inline const std::vector<std::string>&
  getEndorseList() const
  { return m_endorseList; }

  inline virtual ndn::Name
  getPublicKeyName () const
  { return m_keyName; }

protected:
  ndn::Name m_keyName;
  ndn::Name m_signer;
  ndn::Ptr<ProfileData> m_profileData;
  std::vector<std::string> m_endorseList;
};

#endif
