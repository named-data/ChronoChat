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

using namespace std;
using namespace ndn;

namespace chronos{

const OID EndorseCertificate::PROFILE_EXT_OID("1.3.6.1.5.32.2.1");
const OID EndorseCertificate::ENDORSE_EXT_OID("1.3.6.1.5.32.2.2");

const std::vector<std::string> EndorseCertificate::DEFAULT_ENDORSE_LIST = std::vector<std::string>();

Chronos::EndorseExtensionMsg&
operator << (Chronos::EndorseExtensionMsg& endorseExtension, const vector<string>& endorseList)
{ 
  vector<string>::const_iterator it = endorseList.begin();
  for(; it != endorseList.end(); it++)
    endorseExtension.add_endorseentry()->set_name(*it);

  return endorseExtension;
}

Chronos::EndorseExtensionMsg&
operator >> (Chronos::EndorseExtensionMsg& endorseExtension, vector<string>& endorseList)
{
  for(int i = 0; i < endorseExtension.endorseentry_size(); i ++)
    endorseList.push_back(endorseExtension.endorseentry(i).name());

  return endorseExtension;
}

EndorseCertificate::EndorseCertificate(const IdentityCertificate& kskCertificate,
                                       const Profile& profile,
                                       const vector<string>& endorseList)
  : Certificate()
  , m_profile(profile)
  , m_endorseList(endorseList)
{
  m_keyName = IdentityCertificate::certificateNameToPublicKeyName(kskCertificate.getName());
  m_signer = m_keyName;

  Name dataName = m_keyName;
  dataName.append("PROFILE-CERT").append(m_signer.wireEncode()).appendVersion();
  setName(dataName);

  setNotBefore(kskCertificate.getNotBefore());
  setNotAfter(kskCertificate.getNotAfter());
  addSubjectDescription(CertificateSubjectDescription("2.5.4.41", m_keyName.toUri()));
  setPublicKeyInfo(kskCertificate.getPublicKeyInfo());  

  OBufferStream profileStream;
  m_profile.encode(profileStream);
  addExtension(CertificateExtension(PROFILE_EXT_OID, true, *profileStream.buf()));

  OBufferStream endorseStream;
  Chronos::EndorseExtensionMsg endorseExtension;
  endorseExtension << m_endorseList;
  endorseExtension.SerializeToOstream(&endorseStream);
  addExtension(CertificateExtension(ENDORSE_EXT_OID, true, *endorseStream.buf()));
  
  encode();
}

EndorseCertificate::EndorseCertificate(const EndorseCertificate& endorseCertificate,
                                       const Name& signer,
                                       const vector<string>& endorseList)
  : Certificate()
  , m_keyName(endorseCertificate.m_keyName)
  , m_signer(signer)
  , m_profile(endorseCertificate.m_profile)
  , m_endorseList(endorseList)
{
  Name dataName = m_keyName;
  dataName.append("PROFILE-CERT").append(m_signer.wireEncode()).appendVersion();
  setName(dataName);
  
  setNotBefore(endorseCertificate.getNotBefore());
  setNotAfter(endorseCertificate.getNotAfter());
  addSubjectDescription(CertificateSubjectDescription("2.5.4.41", m_keyName.toUri()));
  setPublicKeyInfo(endorseCertificate.getPublicKeyInfo());

  OBufferStream profileStream;
  m_profile.encode(profileStream);
  addExtension(CertificateExtension(PROFILE_EXT_OID, true, *profileStream.buf()));

  OBufferStream endorseStream;
  Chronos::EndorseExtensionMsg endorseExtension;
  endorseExtension << m_endorseList;
  endorseExtension.SerializeToOstream(&endorseStream);
  addExtension(CertificateExtension(ENDORSE_EXT_OID, true, *endorseStream.buf()));

  encode();
}

EndorseCertificate::EndorseCertificate(const Name& keyName,
                                       const PublicKey& key,
                                       const time::system_clock::TimePoint& notBefore,
                                       const time::system_clock::TimePoint& notAfter,
                                       const Name& signer,
                                       const Profile& profile,
                                       const vector<string>& endorseList)
  : Certificate()
  , m_keyName(keyName)
  , m_signer(signer)
  , m_profile(profile)
  , m_endorseList(endorseList)
{
  Name dataName = m_keyName;
  dataName.append("PROFILE-CERT").append(m_signer.wireEncode()).appendVersion();
  setName(dataName);
  
  setNotBefore(notBefore);
  setNotAfter(notAfter);
  addSubjectDescription(CertificateSubjectDescription("2.5.4.41", m_keyName.toUri()));
  setPublicKeyInfo(key);

  OBufferStream profileStream;
  m_profile.encode(profileStream);
  addExtension(CertificateExtension(PROFILE_EXT_OID, true, *profileStream.buf()));

  OBufferStream endorseStream;
  Chronos::EndorseExtensionMsg endorseExtension;
  endorseExtension << m_endorseList;
  endorseExtension.SerializeToOstream(&endorseStream);
  addExtension(CertificateExtension(ENDORSE_EXT_OID, true, *endorseStream.buf()));

  encode();  
}

EndorseCertificate::EndorseCertificate(const EndorseCertificate& endorseCertificate)
  : Certificate(endorseCertificate)
  , m_keyName(endorseCertificate.m_keyName)
  , m_signer(endorseCertificate.m_signer)
  , m_profile(endorseCertificate.m_profile)
  , m_endorseList(endorseCertificate.m_endorseList)
{}

EndorseCertificate::EndorseCertificate(const Data& data)
  : Certificate(data)
{
  const Name& dataName = data.getName();

  if(dataName.size() < 3 || dataName.get(-3).toEscapedString() != "PROFILE-CERT")
    throw Error("No PROFILE-CERT component in data name!");    

  m_keyName = dataName.getPrefix(-3);
  m_signer.wireDecode(dataName.get(-2).blockFromValue());

  ExtensionList::iterator it = extensionList_.begin();
  for(; it != extensionList_.end(); it++)
    {
      if(PROFILE_EXT_OID == it->getOid())
	{
          boost::iostreams::stream<boost::iostreams::array_source> is 
            (reinterpret_cast<const char*>(it->getValue().buf()), it->getValue().size());
	  m_profile.decode(is);
	}
      if(ENDORSE_EXT_OID == it->getOid())
        {
          Chronos::EndorseExtensionMsg endorseExtension;

          boost::iostreams::stream<boost::iostreams::array_source> is 
            (reinterpret_cast<const char*>(it->getValue().buf()), it->getValue().size());          
          endorseExtension.ParseFromIstream(&is);

          endorseExtension >> m_endorseList;
        }
    }
}

}//chronos
