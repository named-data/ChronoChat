/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 *         Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#include "endorse-certificate.hpp"
#include <boost/iostreams/stream.hpp>
#include <ndn-cxx/encoding/buffer-stream.hpp>
#include "endorse-extension.hpp"
#include <list>

namespace chronochat {

using std::vector;
using std::string;

using ndn::PublicKey;
using ndn::IdentityCertificate;
using ndn::CertificateSubjectDescription;
using ndn::CertificateExtension;
using ndn::OID;
using ndn::OBufferStream;

const OID EndorseCertificate::PROFILE_EXT_OID("1.3.6.1.5.32.2.1");
const OID EndorseCertificate::ENDORSE_EXT_OID("1.3.6.1.5.32.2.2");

const vector<string> EndorseCertificate::DEFAULT_ENDORSE_LIST;

EndorseExtension&
operator<<(EndorseExtension& endorseExtension, const vector<string>& endorseList)
{
  for (const auto& entry : endorseList)
    endorseExtension.addEntry(entry);

  return endorseExtension;
}

EndorseExtension&
operator>>(EndorseExtension& endorseExtension, vector<string>& endorseList)
{
  const std::list<string>& endorseEntries = endorseExtension.getEntries();
  for (const auto& entry: endorseEntries)
    endorseList.push_back(entry);

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
  addSubjectDescription(CertificateSubjectDescription(OID("2.5.4.41"), m_keyName.toUri()));
  setPublicKeyInfo(kskCertificate.getPublicKeyInfo());

  Block profileWire = m_profile.wireEncode();
  addExtension(CertificateExtension(PROFILE_EXT_OID, true, ndn::Buffer(profileWire.wire(),
                                                                       profileWire.size())));

  EndorseExtension endorseExtension;
  endorseExtension << m_endorseList;
  Block endorseWire = endorseExtension.wireEncode();
  addExtension(CertificateExtension(ENDORSE_EXT_OID, true, ndn::Buffer(endorseWire.wire(),
                                                                       endorseWire.size())));

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
  addSubjectDescription(CertificateSubjectDescription(OID("2.5.4.41"), m_keyName.toUri()));
  setPublicKeyInfo(endorseCertificate.getPublicKeyInfo());

  Block profileWire = m_profile.wireEncode();
  addExtension(CertificateExtension(PROFILE_EXT_OID, true, ndn::Buffer(profileWire.wire(),
                                                                       profileWire.size())));

  EndorseExtension endorseExtension;
  endorseExtension << m_endorseList;
  Block endorseWire = endorseExtension.wireEncode();
  addExtension(CertificateExtension(ENDORSE_EXT_OID, true, ndn::Buffer(endorseWire.wire(),
                                                                       endorseWire.size())));

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
  addSubjectDescription(CertificateSubjectDescription(OID("2.5.4.41"), m_keyName.toUri()));
  setPublicKeyInfo(key);

  Block profileWire = m_profile.wireEncode();
  addExtension(CertificateExtension(PROFILE_EXT_OID, true, ndn::Buffer(profileWire.wire(),
                                                                       profileWire.size())));

  EndorseExtension endorseExtension;
  endorseExtension << m_endorseList;
  Block endorseWire = endorseExtension.wireEncode();
  addExtension(CertificateExtension(ENDORSE_EXT_OID, true, ndn::Buffer(endorseWire.wire(),
                                                                       endorseWire.size())));

  encode();
}

EndorseCertificate::EndorseCertificate(const EndorseCertificate& endorseCertificate)
  : Certificate(endorseCertificate)
  , m_keyName(endorseCertificate.m_keyName)
  , m_signer(endorseCertificate.m_signer)
  , m_profile(endorseCertificate.m_profile)
  , m_endorseList(endorseCertificate.m_endorseList)
{
}

EndorseCertificate::EndorseCertificate(const Data& data)
  : Certificate(data)
{
  const Name& dataName = data.getName();

  if(dataName.size() < 3 || dataName.get(-3).toUri() != "PROFILE-CERT")
    throw Error("No PROFILE-CERT component in data name!");

  m_keyName = dataName.getPrefix(-3);
  m_signer.wireDecode(dataName.get(-2).blockFromValue());


  for (const auto& entry : m_extensionList) {
    if (PROFILE_EXT_OID == entry.getOid()) {
      m_profile.wireDecode(Block(entry.getValue().buf(), entry.getValue().size()));
    }
    if (ENDORSE_EXT_OID == entry.getOid()) {
      EndorseExtension endorseExtension;
      endorseExtension.wireDecode(Block(entry.getValue().buf(), entry.getValue().size()));

      endorseExtension >> m_endorseList;
    }
  }
}

} // namespace chronochat
