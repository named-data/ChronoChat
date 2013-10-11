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

using namespace std;
using namespace ndn;

ProfileData::ProfileData (const Name& identityName,
			  const string& profileType,
			  const Blob& profileValue)
  : Data()
  , m_identityName(identityName)
  , m_profileType(profileType)
{
  Name tmpName = identityName;
  setName(tmpName.append(profileType));
  setContent(Content(profileValue.buf(), profileValue.size()));
}


ProfileData::ProfileData (const ProfileData& profile)
  : Data()
  , m_identityName(profile.m_identityName)
  , m_profileType(profile.m_profileType)
{
  setName(profile.getName());
  setContent(profile.getContent());
}
 
ProfileData::ProfileData (const Data& data)
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

  if(profileIndex < 0 || profileIndex + 1 >= dataName.size())
    throw LnException("No PROFILE component in data name!");
  
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

  m_identityName = dataName.getSubName(0, profileIndex);
  m_profileType = dataName.get(profileIndex+1).toUri();
}

