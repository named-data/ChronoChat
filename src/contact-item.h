/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef LINKNDN_CONTACT_ITEM_H
#define LINKNDN_CONTACT_ITEM_H

#include <ndn-cpp-dev/data.hpp>
#include <ndn-cpp-et/regex/regex.hpp>
#include <vector>
#include "endorse-certificate.h"

class ContactItem
{
  typedef std::vector<ndn::ptr_lib::shared_ptr<EndorseCertificate> > EndorseCertificateList;

public:
  ContactItem(const EndorseCertificate& selfEndorseCertificate,
              bool isIntroducer = false,
              const std::string& alias = std::string());

  ContactItem(const ContactItem& contactItem);
  
  virtual
  ~ContactItem() {}

  const EndorseCertificate&
  getSelfEndorseCertificate() const
  { return m_selfEndorseCertificate; }

  const ndn::Name&
  getNameSpace() const
  { return m_namespace; }

  const std::string&
  getAlias() const
  { return m_alias; }

  const std::string&
  getName() const
  { return m_name; }

  const std::string&
  getInstitution() const
  { return m_institution; }

  const ndn::Name
  getPublicKeyName() const
  { return m_selfEndorseCertificate.getPublicKeyName(); }

  bool
  isIntroducer() const
  { return m_isIntroducer; }

  void
  setIsIntroducer(bool isIntroducer) 
  { m_isIntroducer = isIntroducer; }

  void
  addTrustScope(const ndn::Name& nameSpace)
  {
    m_trustScopeName.push_back(nameSpace);
    m_trustScope.push_back(ndn::Regex::fromName(nameSpace)); 
  }

  bool
  canBeTrustedFor(const ndn::Name& name);

  const std::vector<ndn::Name>&
  getTrustScopeList() const
  { return m_trustScopeName; }

protected:
  EndorseCertificate m_selfEndorseCertificate;

  ndn::Name m_namespace;
  std::string m_alias;

  std::string m_name;
  std::string m_institution;

  bool m_isIntroducer;

  std::vector<ndn::ptr_lib::shared_ptr<ndn::Regex> > m_trustScope;
  std::vector<ndn::Name> m_trustScopeName;
};

#endif
