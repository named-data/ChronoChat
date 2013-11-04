/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "trusted-contact.h"
#include <boost/tokenizer.hpp>
using boost::tokenizer;
using boost::escaped_list_separator;

using namespace std;
using namespace ndn;

TrustedContact::TrustedContact(const EndorseCertificate& selfEndorseCertificate,
			       const string& trustScope,
			       const string& alias)
  : ContactItem(selfEndorseCertificate, alias)
{
  m_isIntroducer = true;

  tokenizer<escaped_list_separator<char> > trustScopeItems(trustScope, escaped_list_separator<char> ("\\", " \t", "'\""));

  tokenizer<escaped_list_separator<char> >::iterator it = trustScopeItems.begin();

  while (it != trustScopeItems.end())
    {
      m_trustScope.push_back(Regex::fromName(Name(*it)));
      m_trustScopeName.push_back(Name(*it));
      it++;
    }
}

TrustedContact::TrustedContact(const ContactItem& contactItem)
  : ContactItem(contactItem)
{
  m_isIntroducer = true;
}

TrustedContact::TrustedContact(const TrustedContact& trustedContact)
  : ContactItem(trustedContact)
  , m_trustScope(trustedContact.m_trustScope)
  , m_trustScopeName(trustedContact.m_trustScopeName)
{
  m_isIntroducer = true;
}

bool 
TrustedContact::canBeTrustedFor(const Name& name)
{
  vector<Ptr<Regex> >::iterator it = m_trustScope.begin();

  for(; it != m_trustScope.end(); it++)
    if((*it)->match(name))
      return true;
  return false;
}

Ptr<Blob> 
TrustedContact::getTrustScopeBlob() const
{
  ostringstream oss;

  vector<Name>::const_iterator it = m_trustScopeName.begin();
  if(it != m_trustScopeName.end())
    oss << it->toUri();
  for(; it != m_trustScopeName.end(); it++)
    oss << " " << it->toUri();

  return Ptr<Blob>(new Blob(oss.str().c_str(), oss.str().size()));
}

