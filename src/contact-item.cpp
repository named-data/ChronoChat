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
#include "null-ptrs.h"

#include "logging.h"

using namespace std;
using namespace ndn;
using namespace ndn::ptr_lib;

INIT_LOGGER("ContactItem");

ContactItem::ContactItem(const EndorseCertificate& selfEndorseCertificate,
                         bool isIntroducer,
                         const string& alias)
  : m_selfEndorseCertificate(selfEndorseCertificate)
  , m_isIntroducer(isIntroducer)
{
  Name endorsedkeyName = selfEndorseCertificate.getPublicKeyName();

  m_namespace = endorsedkeyName.getSubName(0, endorsedkeyName.size() - 1);


  const ProfileData& profileData = selfEndorseCertificate.getProfileData();
  m_name = profileData.getProfile().getProfileEntry("name");
  m_alias = alias.empty() ? m_name : alias;
  m_institution = profileData.getProfile().getProfileEntry("institution");
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
  vector<shared_ptr<Regex> >::iterator it = m_trustScope.begin();

  for(; it != m_trustScope.end(); it++)
    if((*it)->match(name))
      return true;
  return false;
}
