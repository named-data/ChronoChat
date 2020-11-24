/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2020, Regents of the University of California
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

class EndorseCertificate : public ndn::security::Certificate
{
public:
  class Error : public ndn::security::Certificate::Error
  {
  public:
    Error(const std::string& what)
      : ndn::security::Certificate::Error(what)
    {
    }
  };

  static const std::vector<std::string> DEFAULT_ENDORSE_LIST;

  EndorseCertificate() {}

  EndorseCertificate(const ndn::security::Certificate& kskCertificate,
                     const Profile& profile,
                     const std::vector<std::string>& endorseList = DEFAULT_ENDORSE_LIST);

  EndorseCertificate(const EndorseCertificate& endorseCertificate,
                     const Name& signer,
                     const std::vector<std::string>& endorseList = DEFAULT_ENDORSE_LIST);

  EndorseCertificate(const Name& keyName,
                     const ndn::Buffer& key,
                     const time::system_clock::TimePoint& notBefore,
                     const time::system_clock::TimePoint& notAfter,
                     const Name::Component& signerKeyId,
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

private:
  Name m_signer;
  Profile m_profile;
  std::vector<std::string> m_endorseList;
};

} // namespace chronochat

#endif // CHRONOCHAT_ENDORSE_CERTIFICATE_HPP
