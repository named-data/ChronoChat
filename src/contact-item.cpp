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

#include "logging.h"

using namespace std;
using namespace ndn;

INIT_LOGGER("ContactItem");

namespace chronos{

ContactItem::ContactItem(const EndorseCertificate& selfEndorseCertificate,
                         bool isIntroducer,
                         const string& alias)
  : m_selfEndorseCertificate(selfEndorseCertificate)
  , m_isIntroducer(isIntroducer)
{
  Name endorsedkeyName = selfEndorseCertificate.getPublicKeyName();

  m_namespace = endorsedkeyName.getSubName(0, endorsedkeyName.size() - 1);


  m_name = selfEndorseCertificate.getProfile().get("name");
  m_alias = alias.empty() ? m_name : alias;
  m_institution = selfEndorseCertificate.getProfile().get("institution");
}

ContactItem::ContactItem(const ContactItem& contactItem)
  : m_selfEndorseCertificate(contactItem.m_selfEndorseCertificate)
  , m_namespace(contactItem.m_namespace)
  , m_alias(contactItem.m_alias)
  , m_name(contactItem.m_name)
  , m_institution(contactItem.m_institution)
  , m_isIntroducer(contactItem.m_isIntroducer)
  , m_trustScope(contactItem.m_trustScope)
{}

}
