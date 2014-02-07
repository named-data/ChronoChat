/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOS_CONTACT_ITEM_H
#define CHRONOS_CONTACT_ITEM_H

#include "endorse-certificate.h"
#include <ndn-cpp-dev/util/regex.hpp>
#include <vector>


namespace chronos{

class ContactItem
{
public:
  typedef std::map<ndn::Name, ndn::shared_ptr<ndn::Regex> >::const_iterator const_iterator;
  typedef std::map<ndn::Name, ndn::shared_ptr<ndn::Regex> >::iterator iterator;

  ContactItem(const EndorseCertificate& selfEndorseCertificate,
              bool isIntroducer = false,
              const std::string& alias = "");

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

  const ndn::Name&
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
  { m_trustScope[nameSpace] = ndn::Regex::fromName(nameSpace); }

  void
  deleteTrustScope(const ndn::Name& nameSpace)
  {
    std::map<ndn::Name, ndn::shared_ptr<ndn::Regex> >::iterator it = m_trustScope.find(nameSpace);
    if(it != m_trustScope.end())
      m_trustScope.erase(it);
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
  { return m_trustScope.begin(); }

  const_iterator
  trustScopeEnd() const
  { return m_trustScope.end(); }

  iterator
  trustScopeBegin()
  { return m_trustScope.begin(); }

  iterator
  trustScopeEnd()
  { return m_trustScope.end(); }


protected:
  EndorseCertificate m_selfEndorseCertificate;

  ndn::Name m_namespace;
  std::string m_alias;

  std::string m_name;
  std::string m_institution;

  bool m_isIntroducer;

  std::map<ndn::Name, ndn::shared_ptr<ndn::Regex> > m_trustScope;
};

}//chronos

#endif
