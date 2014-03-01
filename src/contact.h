/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOS_CONTACT_H
#define CHRONOS_CONTACT_H

#include <ndn-cpp-dev/security/identity-certificate.hpp>
#include <ndn-cpp-dev/util/regex.hpp>
#include "endorse-certificate.h"
#include <vector>

namespace chronos{

class Contact
{
public:
  typedef std::map<ndn::Name, ndn::shared_ptr<ndn::Regex> >::const_iterator const_iterator;
  typedef std::map<ndn::Name, ndn::shared_ptr<ndn::Regex> >::iterator iterator;

  Contact(const ndn::IdentityCertificate& identityCertificate,
          bool isIntroducer = false,
          const std::string& alias = "")
    : m_notBefore(identityCertificate.getNotBefore())
    , m_notAfter(identityCertificate.getNotAfter())
    , m_isIntroducer(isIntroducer)
    , m_profile(identityCertificate)
  {
    m_name = m_profile.get("name");
    m_alias = alias.empty() ? m_name : alias;
    m_institution = m_profile.get("institution");
    
    m_keyName = identityCertificate.getPublicKeyName();
    m_namespace = m_keyName.getPrefix(-1);
    m_publicKey = identityCertificate.getPublicKeyInfo();
  }

  Contact(const EndorseCertificate& endorseCertificate,
          bool isIntroducer = false,
          const std::string& alias = "")
    : m_notBefore(endorseCertificate.getNotBefore())
    , m_notAfter(endorseCertificate.getNotAfter())
    , m_isIntroducer(isIntroducer)
  { 
    m_profile = endorseCertificate.getProfile();
    
    m_name = m_profile.get("name");
    m_alias = alias.empty() ? m_name : alias;
    m_institution = m_profile.get("institution");
    
    m_keyName = endorseCertificate.getPublicKeyName();;
    m_namespace = m_keyName.getPrefix(-1);
    m_publicKey = endorseCertificate.getPublicKeyInfo();
  }

  Contact(const ndn::Name& identity,
          const std::string& alias,
          const ndn::Name& keyName,
          ndn::MillisecondsSince1970 notBefore,
          ndn::MillisecondsSince1970 notAfter,
          const ndn::PublicKey& key,
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
  {}
  
  virtual
  ~Contact()
  {}

  const ndn::Name&
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

  const ndn::Name&
  getPublicKeyName() const
  {
    return m_keyName; 
  }

  const ndn::PublicKey&
  getPublicKey() const
  {
    return m_publicKey;
  }

  ndn::MillisecondsSince1970
  getNotBefore() const
  {
    return m_notBefore;
  }

  ndn::MillisecondsSince1970
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
  addTrustScope(const ndn::Name& nameSpace)
  { 
    m_trustScope[nameSpace] = ndn::Regex::fromName(nameSpace); 
  }

  void
  deleteTrustScope(const ndn::Name& nameSpace)
  {
    m_trustScope.erase(nameSpace);
  }

  bool
  canBeTrustedFor(const ndn::Name& name)
  {
    std::map<ndn::Name, ndn::shared_ptr<ndn::Regex> >::iterator it = m_trustScope.begin();

    for(; it != m_trustScope.end(); it++)
      if(it->second->match(name))
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
  typedef std::map<ndn::Name, ndn::shared_ptr<ndn::Regex> > TrustScopes;

  ndn::Name m_namespace;
  std::string m_alias;
  std::string m_name;
  std::string m_institution;
  ndn::Name m_keyName;
  ndn::PublicKey m_publicKey;
  ndn::MillisecondsSince1970 m_notBefore;
  ndn::MillisecondsSince1970 m_notAfter;

  bool m_isIntroducer;
  Profile m_profile;
  
  TrustScopes m_trustScope;
};

} // namespace chronos

#endif // CHRONOS_CONTACT_H
