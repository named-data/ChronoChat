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

  m_namespace = endorsedkeyName.getSubName(0, endorsedkeyName.size() - 1);


  Ptr<ProfileData> profileData = selfEndorseCertificate.getProfileData();
  Ptr<const Blob> nameBlob = profileData->getProfile().getProfileEntry("name");
  m_name = string(nameBlob->buf(), nameBlob->size());
  m_alias = alias.empty() ? m_name : alias;
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
