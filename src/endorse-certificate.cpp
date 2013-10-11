/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "endorse-certificate.h"
#include "exception.h"
#include <ndn.cxx/security/certificate/certificate-subdescrpt.h>


using namespace std;
using namespace ndn;
using namespace ndn::security;

EndorseExtension::EndorseExtension(const Blob & value)
  : CertificateExtension("1.3.6.1.5.32.2", true, value)
{}

EndorseCertificate::EndorseCertificate(const IdentityCertificate& kskCertificate,
				       const Name& signer,
				       const Time& notBefore,
				       const Time& notAfter)
  : Certificate()
  , m_keyName(kskCertificate.getPublicKeyName())
  , m_signer(signer)
{
  setNotBefore(notBefore);
  setNotAfter(notAfter);
  addSubjectDescription(CertificateSubDescrypt("2.5.4.41", m_keyName.toUri()));
  setPublicKeyInfo(kskCertificate.getPublicKeyInfo());
}

EndorseCertificate::EndorseCertificate(const EndorseCertificate& endorseCertificate)
  : Certificate(endorseCertificate)
  , m_keyName(endorseCertificate.m_keyName)
  , m_signer(endorseCertificate.m_signer)
  , m_profileList(endorseCertificate.m_profileList)
{}

EndorseCertificate::EndorseCertificate(const Data& data)
  : Certificate(data)
{
  const Name& dataName = data.getName();
  name::Component certFlag(string("PROFILE-CERT"));  

  int profileIndex = -1;
  for(int i = 0; i < dataName.size(); i++)
    {
      if(0 == dataName.get(i).compare(certFlag))
	{
	  profileIndex = i;
	  break;
	}
    }

  if(profileIndex < 0)
    throw LnException("No PROFILE-CERT component in data name!");

  m_keyName = dataName.getSubName(0, profileIndex);
  m_signer = dataName.getSubName(profileIndex + 1, dataName.size() - profileIndex - 2);

  OID profileExtenstionOID("1.3.6.1.5.32.2");
  ExtensionList::iterator it = m_extnList.begin();
  for(; it != m_extnList.end(); it++)
    {
      if(profileExtenstionOID == it->getOID())
	{
	  Ptr<Blob> valueBlob = Ptr<Blob>(new Blob(it->getValue().buf(), it->getValue().size()));
	  m_profileList.push_back(ProfileData(*(Data::decodeFromWire(valueBlob))));
	}
    }
}
