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
#include "endorse-extension.pb.h"
#include <boost/iostreams/stream.hpp>
#include "logging.h"

using namespace std;
using namespace ndn;
using namespace ndn::ptr_lib;

INIT_LOGGER("EndorseCertificate");

ProfileExtension::ProfileExtension(const ProfileData & profileData)
  : CertificateExtension("1.3.6.1.5.32.2.1", true, Buffer(profileData.wireEncode().wire(), profileData.wireEncode().size()))
{}

ProfileExtension::ProfileExtension(const ProfileExtension& profileExtension)
  : CertificateExtension("1.3.6.1.5.32.2.1", true, profileExtension.extensionValue_)
{}

ProfileExtension::ProfileExtension(const CertificateExtension& extension)
  : CertificateExtension(extension.getOid(), extension.getIsCritical(), extension.getValue())
{
  if(extensionId_ != OID("1.3.6.1.5.32.2.1"))
    throw Error("Wrong ProfileExtension Number!");
}

shared_ptr<ProfileData>
ProfileExtension::getProfileData()
{
  Data data;
  data.wireDecode(Block(extensionValue_.buf(), extensionValue_.size()));
  return make_shared<ProfileData>(data);
}

EndorseExtension::EndorseExtension(const vector<string>& endorseList)
  : CertificateExtension("1.3.6.1.5.32.2.2", true, EndorseExtension::encodeEndorseList(endorseList))
{}

EndorseExtension::EndorseExtension(const EndorseExtension& endorseExtension)
  : CertificateExtension("1.3.6.1.5.32.2.2", true, endorseExtension.extensionValue_)
{}

EndorseExtension::EndorseExtension(const CertificateExtension& extension)
  : CertificateExtension(extension.getOid(), extension.getIsCritical(), extension.getValue())
{
  if(extensionId_ != OID("1.3.6.1.5.32.2.2"))
    throw Error("Wrong EndorseExtension Number!");
}

vector<string>
EndorseExtension::getEndorseList()
{
  Chronos::EndorseExtensionMsg endorseExtension;

  boost::iostreams::stream
    <boost::iostreams::array_source> is ((const char*)extensionValue_.buf(), extensionValue_.size());

  endorseExtension.ParseFromIstream(&is);

  vector<string> endorseList;

  for(int i = 0; i < endorseExtension.endorseentry_size(); i ++)
    endorseList.push_back(endorseExtension.endorseentry(i).name());
  
  return endorseList;
}

Buffer
EndorseExtension::encodeEndorseList(const vector<string>& endorseList)
{
  Chronos::EndorseExtensionMsg endorseExtension;
  
  vector<string>::const_iterator it = endorseList.begin();
  for(; it != endorseList.end(); it++)
    endorseExtension.add_endorseentry()->set_name(*it);

  string encoded;
  endorseExtension.SerializeToString(&encoded);
  
  return Buffer(encoded.c_str(), encoded.size());
}

EndorseCertificate::EndorseCertificate(const IdentityCertificate& kskCertificate,
                                       const ProfileData& profileData,
                                       const vector<string>& endorseList)
  : Certificate()
  , m_keyName(kskCertificate.getPublicKeyName())
  , m_signer(kskCertificate.getPublicKeyName())
  , m_profileData(profileData)
  , m_endorseList(endorseList)
{
  Name dataName = m_keyName;
  dataName.append("PROFILE-CERT").append(m_signer.wireEncode()).appendVersion();
  setName(dataName);

  setNotBefore(kskCertificate.getNotBefore());
  setNotAfter(kskCertificate.getNotAfter());
  addSubjectDescription(CertificateSubjectDescription("2.5.4.41", m_keyName.toUri()));
  setPublicKeyInfo(kskCertificate.getPublicKeyInfo());  
  addExtension(ProfileExtension(m_profileData));
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
  dataName.append("PROFILE-CERT").append(m_signer.wireEncode()).appendVersion();
  setName(dataName);
  
  setNotBefore(endorseCertificate.getNotBefore());
  setNotAfter(endorseCertificate.getNotAfter());
  addSubjectDescription(CertificateSubjectDescription("2.5.4.41", m_keyName.toUri()));
  setPublicKeyInfo(endorseCertificate.getPublicKeyInfo());
  addExtension(ProfileExtension(m_profileData));
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

  if(dataName.size() < 3 || !dataName.get(-3).equals("PROFILE-CERT"))
    throw Error("No PROFILE-CERT component in data name!");    

  m_keyName = dataName.getPrefix(-3);
  m_signer.wireDecode(Block(dataName.get(-2).getValue().buf(),
                            dataName.get(-2).getValue().size()));

  OID profileExtensionOID("1.3.6.1.5.32.2.1");
  OID endorseExtensionOID("1.3.6.1.5.32.2.2");

  ExtensionList::iterator it = extensionList_.begin();
  for(; it != extensionList_.end(); it++)
    {
      if(profileExtensionOID == it->getOid())
	{
          ProfileExtension profileExtension(*it);
	  m_profileData = *profileExtension.getProfileData();
	}
      if(endorseExtensionOID == it->getOid())
        {
          EndorseExtension endorseExtension(*it);
          m_endorseList = endorseExtension.getEndorseList();
        }
    }
}
