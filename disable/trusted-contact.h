/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef LINKNDN_TRUSTED_CONTACT_H
#define LINKNDN_TRUSTED_CONTACT_H

#include "contact-item.h"
#include <ndn.cxx/regex/regex.h>

class TrustedContact : public virtual ContactItem
{
public:
  TrustedContact(const EndorseCertificate& selfEndorseCertificate,
                 const std::string& trustScope,
                 const std::string& alias = std::string());

  TrustedContact(const ContactItem& contactItem);

  TrustedContact(const TrustedContact& trustedContact);
 
  virtual
  ~TrustedContact() {}

  void
  addTrustScope(ndn::Ptr<ndn::Regex> nameSpace)
  { m_trustScope.push_back(nameSpace); }

  bool
  canBeTrustedFor(const ndn::Name& name);

  ndn::Ptr<ndn::Blob> 
  getTrustScopeBlob() const;

  inline const std::vector<ndn::Name>&
  getTrustScopeList() const
  { return m_trustScopeName; }

private:
  std::vector<ndn::Ptr<ndn::Regex> > m_trustScope;
  std::vector<ndn::Name> m_trustScopeName;
};

#endif
