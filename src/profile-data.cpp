/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "profile-data.h"
#include "exception.h"
#include <ndn.cxx/fields/signature-sha256-with-rsa.h>
#include "logging.h"


using namespace ndn;
using namespace std;

INIT_LOGGER("ProfileData");

ProfileData::ProfileData(const Profile& profile)
  : Data()
  , m_identity(profile.getIdentityName())
  , m_profile(profile)
{
  Name dataName = m_identity;

  dataName.append("PROFILE").appendVersion();
  setName(dataName);
  Ptr<Blob> profileBlob = profile.toDerBlob();
  setContent(Content(profileBlob->buf(), profileBlob->size()));
}

ProfileData::ProfileData(const ProfileData& profileData)
  : Data()
  , m_identity(profileData.m_identity)
  , m_profile(profileData.m_profile)
{
  Ptr<const signature::Sha256WithRsa> dataSig = boost::dynamic_pointer_cast<const signature::Sha256WithRsa>(profileData.getSignature());
  Ptr<signature::Sha256WithRsa> newSig = Ptr<signature::Sha256WithRsa>::Create();

  Ptr<SignedBlob> newSignedBlob = NULL;
  if(profileData.getSignedBlob() != NULL)
    newSignedBlob = Ptr<SignedBlob>(new SignedBlob(*profileData.getSignedBlob()));
  
  newSig->setKeyLocator(dataSig->getKeyLocator());
  newSig->setPublisherKeyDigest(dataSig->getPublisherKeyDigest());
  newSig->setSignatureBits(dataSig->getSignatureBits());
  
  setName(profileData.getName());
  setSignature(newSig);
  setContent(profileData.getContent());
  setSignedBlob(newSignedBlob);
}

ProfileData::ProfileData(const Data& data)
  : Data()
{
  const Name& dataName = data.getName();
  name::Component appFlag(string("PROFILE"));  

  int profileIndex = -1;
  for(int i = 0; i < dataName.size(); i++)
    {
      if(0 == dataName.get(i).compare(appFlag))
	{
	  profileIndex = i;
	  break;
	}
    }

  if(profileIndex < 0)
    throw LnException("No PROFILE component in data name!");

  m_identity = dataName.getSubName(0, profileIndex);

  Ptr<const signature::Sha256WithRsa> dataSig = boost::dynamic_pointer_cast<const signature::Sha256WithRsa>(data.getSignature());
  Ptr<signature::Sha256WithRsa> newSig = Ptr<signature::Sha256WithRsa>::Create();
  
  Ptr<SignedBlob> newSignedBlob = NULL;
  if(data.getSignedBlob() != NULL)
    newSignedBlob = Ptr<SignedBlob>(new SignedBlob(*data.getSignedBlob()));
    
  newSig->setKeyLocator(dataSig->getKeyLocator());
  newSig->setPublisherKeyDigest(dataSig->getPublisherKeyDigest());
  newSig->setSignatureBits(dataSig->getSignatureBits());
  
  setName(data.getName());
  setSignature(newSig);
  setContent(data.getContent());
  setSignedBlob(newSignedBlob);

  m_profile = *Profile::fromDerBlob(data.content());

}
