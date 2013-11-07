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
#include <ndn.cxx/helpers/der/der.h>
#include <ndn.cxx/helpers/der/visitor/simple-visitor.h>
#include <ndn.cxx/security/certificate/certificate-subdescrpt.h>
#include "logging.h"

using namespace std;
using namespace ndn;
using namespace ndn::security;

INIT_LOGGER("EndorseCertificate");

ProfileExtension::ProfileExtension(const ProfileData & profileData)
  : CertificateExtension("1.3.6.1.5.32.2.1", true, *profileData.encodeToWire())
{}

ProfileExtension::ProfileExtension(const ProfileExtension& profileExtension)
  : CertificateExtension("1.3.6.1.5.32.2.1", true, profileExtension.m_extnValue)
{}

ProfileExtension::ProfileExtension(const CertificateExtension& extension)
  : CertificateExtension(extension.getOID(), extension.getCritical(), extension.getValue())
{
  if(m_extnID != OID("1.3.6.1.5.32.2.1"))
    throw LnException("Wrong ProfileExtension Number!");
}

Ptr<ProfileData>
ProfileExtension::getProfileData()
{
  Ptr<Blob> dataBlob = Ptr<Blob>(new Blob(m_extnValue.buf (), m_extnValue.size ()));
  return Ptr<ProfileData>(new ProfileData(*Data::decodeFromWire(dataBlob)));
}

EndorseExtension::EndorseExtension(const vector<string>& endorsedList)
  : CertificateExtension("1.3.6.1.5.32.2.2", true, *EndorseExtension::prepareValue(endorsedList))
{}

EndorseExtension::EndorseExtension(const EndorseExtension& endorseExtension)
  : CertificateExtension("1.3.6.1.5.32.2.2", true, endorseExtension.m_extnValue)
{}

EndorseExtension::EndorseExtension(const CertificateExtension& extension)
  : CertificateExtension(extension.getOID(), extension.getCritical(), extension.getValue())
{
  if(m_extnID != OID("1.3.6.1.5.32.2.2"))
    throw LnException("Wrong EndorseExtension Number!");
}

vector<string>
EndorseExtension::getEndorsedList()
{
  vector<string> endorsedList;

  boost::iostreams::stream
    <boost::iostreams::array_source> is (m_extnValue.buf(), m_extnValue.size());
  
  Ptr<der::DerSequence> root = DynamicCast<der::DerSequence>(der::DerNode::parse(reinterpret_cast<InputIterator &>(is)));
  const der::DerNodePtrList & children = root->getChildren();
  der::SimpleVisitor simpleVisitor;

  for(int i = 0; i < children.size(); i++)
      endorsedList.push_back(boost::any_cast<string>(children[i]->accept(simpleVisitor)));

  return endorsedList;
}

Ptr<Blob>
EndorseExtension::prepareValue(const vector<string>& endorsedList)
{
  Ptr<der::DerSequence> root = Ptr<der::DerSequence>::Create();
  
  vector<string>::const_iterator it = endorsedList.begin();
  for(; it != endorsedList.end(); it++)
    {
      Ptr<der::DerPrintableString> entry = Ptr<der::DerPrintableString>(new der::DerPrintableString(*it));
      root->addChild(entry);
    }
  
  blob_stream blobStream;
  OutputIterator & start = reinterpret_cast<OutputIterator &> (blobStream);
  root->encode(start);

  return blobStream.buf ();
}

EndorseCertificate::EndorseCertificate(const IdentityCertificate& kskCertificate,
                                       Ptr<ProfileData> profileData,
                                       const vector<string>& endorseList)
  : Certificate()
  , m_keyName(kskCertificate.getPublicKeyName())
  , m_signer(kskCertificate.getPublicKeyName())
  , m_profileData(profileData)
  , m_endorseList(endorseList)
{
  Name dataName = m_keyName;
  dataName.append("PROFILE-CERT").append(m_signer).appendVersion();
  setName(dataName);

  setNotBefore(kskCertificate.getNotBefore());
  setNotAfter(kskCertificate.getNotAfter());
  addSubjectDescription(CertificateSubDescrypt("2.5.4.41", m_keyName.toUri()));
  setPublicKeyInfo(kskCertificate.getPublicKeyInfo());  
  addExtension(ProfileExtension(*m_profileData));
  addExtension(EndorseExtension(m_endorseList));
  
  encode();
}

EndorseCertificate::EndorseCertificate(const EndorseCertificate& endorseCertificate,
                                       const Name& signer,
                                       const vector<string>& endorseList)
  : Certificate()
  , m_keyName(endorseCertificate.m_keyName)
  , m_signer(signer)
  , m_profileData(endorseCertificate.m_profileData)
  , m_endorseList(endorseList)
{
  Name dataName = m_keyName;
  dataName.append("PROFILE-CERT").append(m_signer).appendVersion();
  setName(dataName);
  
  setNotBefore(endorseCertificate.getNotBefore());
  setNotAfter(endorseCertificate.getNotAfter());
  addSubjectDescription(CertificateSubDescrypt("2.5.4.41", m_keyName.toUri()));
  setPublicKeyInfo(endorseCertificate.getPublicKeyInfo());
  addExtension(ProfileExtension(*m_profileData));
  addExtension(EndorseExtension(m_endorseList));

  encode();
}

EndorseCertificate::EndorseCertificate(const EndorseCertificate& endorseCertificate)
  : Certificate(endorseCertificate)
  , m_keyName(endorseCertificate.m_keyName)
  , m_signer(endorseCertificate.m_signer)
  , m_profileData(endorseCertificate.m_profileData)
  , m_endorseList(endorseCertificate.m_endorseList)
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

  OID profileExtensionOID("1.3.6.1.5.32.2.1");
  OID endorseExtensionOID("1.3.6.1.5.32.2.2");

  ExtensionList::iterator it = m_extnList.begin();
  for(; it != m_extnList.end(); it++)
    {
      if(profileExtensionOID == it->getOID())
	{
          ProfileExtension profileExtension(*it);
	  m_profileData = profileExtension.getProfileData();
	}
      if(endorseExtensionOID == it->getOID())
        {
          EndorseExtension endorseExtension(*it);
          m_endorseList = endorseExtension.getEndorsedList();
        }
    }
}
