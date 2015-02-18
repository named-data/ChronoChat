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

#ifndef CHRONOCHAT_ENDORSE_CERTIFICATE_HPP
#define CHRONOCHAT_ENDORSE_CERTIFICATE_HPP

#include "profile.hpp"

namespace chronochat {

class EndorseCertificate : public ndn::Certificate
{
public:
  class Error : public ndn::Certificate::Error
  {
  public:
    Error(const std::string& what)
      : ndn::Certificate::Error(what)
    {
    }
  };

  static const std::vector<std::string> DEFAULT_ENDORSE_LIST;

  EndorseCertificate() {}

  EndorseCertificate(const ndn::IdentityCertificate& kskCertificate,
                     const Profile& profile,
                     const std::vector<std::string>& endorseList = DEFAULT_ENDORSE_LIST);

  EndorseCertificate(const EndorseCertificate& endorseCertificate,
                     const Name& signer,
                     const std::vector<std::string>& endorseList = DEFAULT_ENDORSE_LIST);

  EndorseCertificate(const Name& keyName,
                     const ndn::PublicKey& key,
                     const time::system_clock::TimePoint& notBefore,
                     const time::system_clock::TimePoint& notAfter,
                     const Name& signer,
                     const Profile& profile,
                     const std::vector<std::string>& endorseList = DEFAULT_ENDORSE_LIST);

  EndorseCertificate(const EndorseCertificate& endorseCertificate);

  EndorseCertificate(const Data& data);

  virtual
  ~EndorseCertificate()
  {
  }

  const Name&
  getSigner() const
  {
    return m_signer;
  }

  const Profile&
  getProfile() const
  {
    return m_profile;
  }

  const std::vector<std::string>&
  getEndorseList() const
  {
    return m_endorseList;
  }

  const Name&
  getPublicKeyName () const
  {
    return m_keyName;
  }

private:
  static const ndn::OID PROFILE_EXT_OID;
  static const ndn::OID ENDORSE_EXT_OID;

  Name m_keyName;
  Name m_signer; // signing key name
  Profile m_profile;
  std::vector<std::string> m_endorseList;

};

} // namespace chronochat

#endif // CHRONOCHAT_ENDORSE_CERTIFICATE_HPP
