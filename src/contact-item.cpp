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

using namespace std;
using namespace ndn;
using namespace ndn::security;

ContactItem::ContactItem(const EndorseCertificate& selfEndorseCertificate,
                         const string& alias)
  : m_selfEndorseCertificate(selfEndorseCertificate)
{
  Name endorsedkeyName = selfEndorseCertificate.getPublicKeyName();
  Ptr<const signature::Sha256WithRsa> endorseSig = boost::dynamic_pointer_cast<const signature::Sha256WithRsa>(selfEndorseCertificate.getSignature());
  const Name& signingKeyName = endorseSig->getKeyLocator().getKeyName();

  if(endorsedkeyName != signingKeyName)
    throw LnException("not a self-claimed");

  m_namespace = endorsedkeyName.getSubName(0, endorsedkeyName.size() - 1);
  m_alias = alias.empty() ? m_namespace.toUri() : alias;

  Ptr<ProfileData> profileData = selfEndorseCertificate.getProfileData();
  Ptr<const Blob> nameBlob = profileData->getProfile().getProfileEntry("name");
  m_name = string(nameBlob->buf(), nameBlob->size());
  Ptr<const Blob> institutionBlob = profileData->getProfile().getProfileEntry("institution");
  m_institution = string(institutionBlob->buf(), institutionBlob->size());
}

