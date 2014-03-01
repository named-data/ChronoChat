/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOS_ENDORSE_CERTIFICATE_H
#define CHRONOS_ENDORSE_CERTIFICATE_H

#include "profile.h"
#include <vector>
#include <ndn-cpp-dev/security/identity-certificate.hpp>



namespace chronos{

class EndorseCertificate : public ndn::Certificate
{
public:
  struct Error : public ndn::Certificate::Error { Error(const std::string &what) : ndn::Certificate::Error(what) {} };

  static const std::vector<std::string> DEFAULT_ENDORSE_LIST;

  EndorseCertificate() {}

  EndorseCertificate(const ndn::IdentityCertificate& kskCertificate,
                     const Profile& profile,
                     const std::vector<std::string>& endorseList = DEFAULT_ENDORSE_LIST);

  EndorseCertificate(const EndorseCertificate& endorseCertificate,
                     const ndn::Name& signer,
                     const std::vector<std::string>& endorseList = DEFAULT_ENDORSE_LIST);

  EndorseCertificate(const ndn::Name& keyName,
                     const ndn::PublicKey& key,
                     ndn::MillisecondsSince1970 notBefore,
                     ndn::MillisecondsSince1970 notAfter,
                     const ndn::Name& signer,
                     const Profile& profile,
                     const std::vector<std::string>& endorseList = DEFAULT_ENDORSE_LIST);

  EndorseCertificate(const EndorseCertificate& endorseCertificate);

  EndorseCertificate(const ndn::Data& data);

  virtual
  ~EndorseCertificate()
  {}

  const ndn::Name&
  getSigner() const
  { return m_signer; }

  const Profile&
  getProfile() const
  { return m_profile; }

  const std::vector<std::string>&
  getEndorseList() const
  { return m_endorseList; }

  const ndn::Name&
  getPublicKeyName () const
  { return m_keyName; }

private:
  static const ndn::OID PROFILE_EXT_OID;
  static const ndn::OID ENDORSE_EXT_OID;

  ndn::Name m_keyName;
  ndn::Name m_signer; // signing key name
  Profile m_profile;
  std::vector<std::string> m_endorseList;
};

}//chronos

#endif
