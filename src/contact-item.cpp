/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "contact-item.h"
#include "exception.h"

#include <ndn.cxx/fields/signature-sha256-with-rsa.h>

#include "logging.h"

using namespace std;
using namespace ndn;
using namespace ndn::security;

INIT_LOGGER("ContactItem");

ContactItem::ContactItem(const EndorseCertificate& selfEndorseCertificate,
                         bool isIntroducer,
                         const string& alias)
  : m_selfEndorseCertificate(selfEndorseCertificate)
  , m_isIntroducer(isIntroducer)
{
  Name endorsedkeyName = selfEndorseCertificate.getPublicKeyName();
  Ptr<const signature::Sha256WithRsa> endorseSig = boost::dynamic_pointer_cast<const signature::Sha256WithRsa>(selfEndorseCertificate.getSignature());
  const Name& signingKeyName = endorseSig->getKeyLocator().getKeyName();
  
  int i = 0;
  int j = -1;
  string keyString("KEY");
  string idString("ID-CERT");
  for(; i < signingKeyName.size(); i++)
    {
      if(keyString == signingKeyName.get(i).toUri())
        j = i;
      if(idString == signingKeyName.get(i).toUri())
        break;
    }

  if(i >= signingKeyName.size() || j < 0)
    throw LnException("Wrong name!");

  Name subName = signingKeyName.getSubName(0, j);
  subName.append(signingKeyName.getSubName(j+1, i-j-1));



  // _LOG_DEBUG("endorsedkeyName " << endorsedkeyName.toUri());
  // _LOG_DEBUG("subKeyName " << subName.toUri());

  if(endorsedkeyName != subName)
    throw LnException("not a self-claimed");

  m_namespace = endorsedkeyName.getSubName(0, endorsedkeyName.size() - 1);
  m_alias = alias.empty() ? m_namespace.toUri() : alias;

  Ptr<ProfileData> profileData = selfEndorseCertificate.getProfileData();
  Ptr<const Blob> nameBlob = profileData->getProfile().getProfileEntry("name");
  m_name = string(nameBlob->buf(), nameBlob->size());
  Ptr<const Blob> institutionBlob = profileData->getProfile().getProfileEntry("institution");
  m_institution = string(institutionBlob->buf(), institutionBlob->size());
}

ContactItem::ContactItem(const ContactItem& contactItem)
  : m_selfEndorseCertificate(contactItem.m_selfEndorseCertificate)
  , m_namespace(contactItem.m_namespace)
  , m_alias(contactItem.m_alias)
  , m_name(contactItem.m_name)
  , m_institution(contactItem.m_institution)
  , m_isIntroducer(contactItem.m_isIntroducer)
  , m_trustScope(contactItem.m_trustScope)
  , m_trustScopeName(contactItem.m_trustScopeName)
{}

bool 
ContactItem::canBeTrustedFor(const Name& name)
{
  vector<Ptr<Regex> >::iterator it = m_trustScope.begin();

  for(; it != m_trustScope.end(); it++)
    if((*it)->match(name))
      return true;
  return false;
}
