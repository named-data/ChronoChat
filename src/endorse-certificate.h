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
#include <ndn-cpp/data.hpp>
#include <ndn-cpp/security/identity-certificate.hpp>
#include <ndn-cpp/security/certificate-extension.hpp>

#include "profile-data.h"

class ProfileExtension : public ndn::CertificateExtension
{
public:
  struct Error : public ndn::CertificateExtension::Error { Error(const std::string &what) : ndn::CertificateExtension::Error(what) {} };

  ProfileExtension(const ProfileData& profileData);
  
  ProfileExtension(const ProfileExtension& profileExtension);

  ProfileExtension(const CertificateExtension& extension);

  ~ProfileExtension() {}

  ndn::ptr_lib::shared_ptr<ProfileData>
  getProfileData();
};

class EndorseExtension : public ndn::CertificateExtension
{
public:
  struct Error : public ndn::CertificateExtension::Error { Error(const std::string &what) : ndn::CertificateExtension::Error(what) {} };

  EndorseExtension(const std::vector<std::string>& endorseList);

  EndorseExtension(const EndorseExtension& endorseExtension);

  EndorseExtension(const CertificateExtension& extension);

  ~EndorseExtension() {}

  std::vector<std::string>
  getEndorseList();

private:
  static ndn::Buffer
  encodeEndorseList(const std::vector<std::string>& endorsedList);
};

class EndorseCertificate : public ndn::Certificate
{
public:
  struct Error : public ndn::Certificate::Error { Error(const std::string &what) : ndn::Certificate::Error(what) {} };

  EndorseCertificate() {}

  EndorseCertificate(const ndn::IdentityCertificate& kskCertificate,
                     const ProfileData& profileData,
                     const std::vector<std::string>& endorseList = std::vector<std::string>());

  EndorseCertificate(const EndorseCertificate& endorseCertificate,
                     const ndn::Name& signer,
                     const std::vector<std::string>& endorseList);

  EndorseCertificate(const EndorseCertificate& endorseCertificate);

  EndorseCertificate(const ndn::Data& data);

  virtual
  ~EndorseCertificate()
  {}

  const ndn::Name&
  getSigner() const
  { return m_signer; }

  const ProfileData&
  getProfileData() const
  { return m_profileData; }

  const std::vector<std::string>&
  getEndorseList() const
  { return m_endorseList; }

  virtual ndn::Name
  getPublicKeyName () const
  { return m_keyName; }

protected:
  ndn::Name m_keyName;
  ndn::Name m_signer;
  ProfileData m_profileData;
  std::vector<std::string> m_endorseList;
};

#endif
