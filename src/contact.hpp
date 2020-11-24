/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2020, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_CONTACT_HPP
#define CHRONOCHAT_CONTACT_HPP

#include "common.hpp"
#include "endorse-certificate.hpp"
#include "profile.hpp"

#include <ndn-cxx/security/certificate.hpp>
#include <ndn-cxx/util/regex.hpp>

namespace chronochat {

class Contact
{
public:
  typedef std::map<Name, shared_ptr<ndn::Regex> >::const_iterator const_iterator;
  typedef std::map<Name, shared_ptr<ndn::Regex> >::iterator iterator;

  Contact(const ndn::security::Certificate& identityCertificate,
          bool isIntroducer = false,
          const std::string& alias = "")
    : m_notBefore(time::system_clock::now())
    , m_notAfter(time::system_clock::now() + time::days(3650))
    , m_isIntroducer(isIntroducer)
    , m_profile(identityCertificate)
  {
    m_keyName = identityCertificate.getKeyName();
    m_namespace = m_keyName.getPrefix(-2);
    m_publicKey = identityCertificate.getPublicKey();

    m_name = m_profile.get("name");
    m_name = m_name.empty() ? m_namespace.toUri() : m_name;
    m_alias = alias.empty() ? m_name : alias;
    m_institution = m_profile.get("institution");

    try {
      m_notBefore = identityCertificate.getValidityPeriod().getPeriod().first;
      m_notAfter = identityCertificate.getValidityPeriod().getPeriod().second;
    } catch (const tlv::Error&) {}
  }

  Contact(const EndorseCertificate& endorseCertificate,
          bool isIntroducer = false,
          const std::string& alias = "")
    : m_notBefore(time::system_clock::now())
    , m_notAfter(time::system_clock::now() + time::days(3650))
    , m_isIntroducer(isIntroducer)
  {
    m_profile = endorseCertificate.getProfile();

    m_keyName = endorseCertificate.getKeyName().getPrefix(-1).append("KEY");
    m_namespace = m_keyName.getPrefix(-3);
    m_publicKey = endorseCertificate.getPublicKey();

    m_name = m_profile.get("name");
    m_name = m_name.empty() ? m_namespace.toUri() : m_name;
    m_alias = alias.empty() ? m_name : alias;
    m_institution = m_profile.get("institution");

    try {
      m_notBefore = endorseCertificate.getValidityPeriod().getPeriod().first;
      m_notAfter = endorseCertificate.getValidityPeriod().getPeriod().second;
    } catch (const tlv::Error&) {}
  }

  Contact(const Name& identity,
          const std::string& alias,
          const Name& keyName,
          const time::system_clock::TimePoint& notBefore,
          const time::system_clock::TimePoint& notAfter,
          const ndn::Buffer& key,
          bool isIntroducer)
    : m_namespace(identity)
    , m_alias(alias)
    , m_keyName(keyName)
    , m_publicKey(key)
    , m_notBefore(notBefore)
    , m_notAfter(notAfter)
    , m_isIntroducer(isIntroducer)
  {
  }

  Contact(const Contact& contact)
    : m_namespace(contact.m_namespace)
    , m_alias(contact.m_alias)
    , m_name(contact.m_name)
    , m_institution(contact.m_institution)
    , m_keyName(contact.m_keyName)
    , m_publicKey(contact.m_publicKey)
    , m_notBefore(contact.m_notBefore)
    , m_notAfter(contact.m_notAfter)
    , m_isIntroducer(contact.m_isIntroducer)
    , m_profile(contact.m_profile)
    , m_trustScope(contact.m_trustScope)
  {
  }

  virtual
  ~Contact()
  {
  }

  const Name&
  getNameSpace() const
  {
    return m_namespace;
  }

  const std::string&
  getAlias() const
  {
    return m_alias;
  }

  const std::string&
  getName() const
  {
    return m_name;
  }

  const std::string&
  getInstitution() const
  {
    return m_institution;
  }

  const Name&
  getPublicKeyName() const
  {
    return m_keyName;
  }

  const ndn::Buffer&
  getPublicKey() const
  {
    return m_publicKey;
  }

  const time::system_clock::TimePoint&
  getNotBefore() const
  {
    return m_notBefore;
  }

  const time::system_clock::TimePoint&
  getNotAfter() const
  {
    return m_notAfter;
  }

  const Profile&
  getProfile() const
  {
    return m_profile;
  }

  void
  setProfile(const Profile& profile)
  {
    m_name = profile.get("name");
    m_institution = profile.get("institution");
    m_profile = profile;
  }

  bool
  isIntroducer() const
  {
    return m_isIntroducer;
  }

  void
  setIsIntroducer(bool isIntroducer)
  {
    m_isIntroducer = isIntroducer;
  }

  void
  addTrustScope(const Name& nameSpace)
  {
    m_trustScope[nameSpace] = ndn::Regex::fromName(nameSpace);
  }

  void
  deleteTrustScope(const Name& nameSpace)
  {
    m_trustScope.erase(nameSpace);
  }

  bool
  canBeTrustedFor(const Name& name)
  {
    for (std::map<Name, shared_ptr<ndn::Regex> >::iterator it = m_trustScope.begin();
         it != m_trustScope.end(); it++)
      if (it->second->match(name))
        return true;
    return false;
  }

  const_iterator
  trustScopeBegin() const
  {
    return m_trustScope.begin();
  }

  const_iterator
  trustScopeEnd() const
  {
    return m_trustScope.end();
  }

  iterator
  trustScopeBegin()
  {
    return m_trustScope.begin();
  }

  iterator
  trustScopeEnd()
  {
    return m_trustScope.end();
  }

protected:
  typedef std::map<Name, shared_ptr<ndn::Regex> > TrustScopes;

  Name m_namespace;
  std::string m_alias;
  std::string m_name;
  std::string m_institution;
  Name m_keyName;
  ndn::Buffer m_publicKey;
  time::system_clock::TimePoint m_notBefore;
  time::system_clock::TimePoint m_notAfter;

  bool m_isIntroducer;
  Profile m_profile;

  TrustScopes m_trustScope;
};

} // namespace chronochat

#endif // CHRONOCHAT_CONTACT_HPP
