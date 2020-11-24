/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2020, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 *         Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#include "endorse-certificate.hpp"
#include "endorse-extension.hpp"

#include <boost/iostreams/stream.hpp>
#include <ndn-cxx/encoding/buffer-stream.hpp>
#include <ndn-cxx/security/additional-description.hpp>
#include <ndn-cxx/security/validity-period.hpp>

namespace chronochat {

using std::vector;
using std::string;

using ndn::security::Certificate;
using ndn::OBufferStream;

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

EndorseCertificate::EndorseCertificate(const Certificate& kskCertificate,
                                       const Profile& profile,
                                       const vector<string>& endorseList)
  : Certificate()
  , m_profile(profile)
  , m_endorseList(endorseList)
{
  setName(kskCertificate.getKeyName().getPrefix(-2)
                        .append("PROFILE-CERT")
                        .append("KEY")
                        .append(kskCertificate.getKeyId())
                        .append("self")
                        .appendTimestamp());

  m_signer = kskCertificate.getKeyName();

  setMetaInfo(kskCertificate.getMetaInfo());
  setContent(kskCertificate.getPublicKey().data(), kskCertificate.getPublicKey().size());

  ndn::security::AdditionalDescription description;
  description.set("2.5.4.41", getKeyName().toUri());
  description.set("signer", m_signer.toUri());

  EndorseExtension endorseExtension;
  endorseExtension << m_endorseList;

  ndn::SignatureInfo signatureInfo;
  signatureInfo.addCustomTlv(description.wireEncode());
  signatureInfo.addCustomTlv(m_profile.wireEncode());

  if (m_endorseList.size() > 0)
    signatureInfo.addCustomTlv(endorseExtension.wireEncode());

  try {
    signatureInfo.setValidityPeriod(kskCertificate.getValidityPeriod());
  } catch (const tlv::Error&) {
    signatureInfo.setValidityPeriod(ndn::security::ValidityPeriod(
      time::system_clock::now(), time::system_clock::now() + time::days(3650)));
  }

  setSignatureInfo(signatureInfo);
}

EndorseCertificate::EndorseCertificate(const EndorseCertificate& endorseCertificate,
                                       const Name& signer,
                                       const vector<string>& endorseList)
  : Certificate()
  , m_signer(signer)
  , m_profile(endorseCertificate.m_profile)
  , m_endorseList(endorseList)
{
  setName(endorseCertificate.getName()
                 .getPrefix(-2)
                 .append(m_signer.wireEncode())
                 .appendVersion());

  setMetaInfo(endorseCertificate.getMetaInfo());
  setContent(endorseCertificate.getPublicKey().data(), endorseCertificate.getPublicKey().size());

  ndn::security::AdditionalDescription description;
  description.set("2.5.4.41", getKeyName().toUri());
  description.set("signer", m_signer.toUri());

  EndorseExtension endorseExtension;
  endorseExtension << m_endorseList;

  ndn::SignatureInfo signatureInfo;
  signatureInfo.addCustomTlv(description.wireEncode());
  signatureInfo.addCustomTlv(m_profile.wireEncode());

  if (m_endorseList.size() > 0)
    signatureInfo.addCustomTlv(endorseExtension.wireEncode());

  try {
    signatureInfo.setValidityPeriod(endorseCertificate.getValidityPeriod());
  } catch (const tlv::Error&) {
    signatureInfo.setValidityPeriod(ndn::security::ValidityPeriod(
      time::system_clock::now(), time::system_clock::now() + time::days(3650)));
  }

  setSignatureInfo(signatureInfo);
}

EndorseCertificate::EndorseCertificate(const Name& keyName,
                                       const ndn::Buffer& key,
                                       const time::system_clock::TimePoint& notBefore,
                                       const time::system_clock::TimePoint& notAfter,
                                       const Name::Component& signerKeyId,
                                       const Name& signer,
                                       const Profile& profile,
                                       const vector<string>& endorseList)
  : Certificate()
  , m_signer(signer)
  , m_profile(profile)
  , m_endorseList(endorseList)
{
  setName(keyName.getPrefix(-2)
                 .append("PROFILE-CERT")
                 .append("KEY")
                 .append(signerKeyId)
                 .append(m_signer.wireEncode())
                 .appendVersion());

  setContent(key.data(), key.size());

  ndn::security::AdditionalDescription description;
  description.set("2.5.4.41", keyName.toUri());
  description.set("signer", m_signer.toUri());

  EndorseExtension endorseExtension;
  endorseExtension << m_endorseList;

  ndn::SignatureInfo signatureInfo;
  signatureInfo.addCustomTlv(description.wireEncode());
  signatureInfo.addCustomTlv(m_profile.wireEncode());

  if (m_endorseList.size() > 0)
    signatureInfo.addCustomTlv(endorseExtension.wireEncode());

  signatureInfo.setValidityPeriod(ndn::security::ValidityPeriod(notBefore, notAfter));

  setSignatureInfo(signatureInfo);
}

EndorseCertificate::EndorseCertificate(const EndorseCertificate& endorseCertificate)
  : Certificate(endorseCertificate)
  , m_signer(endorseCertificate.m_signer)
  , m_profile(endorseCertificate.m_profile)
  , m_endorseList(endorseCertificate.m_endorseList)
{
}

EndorseCertificate::EndorseCertificate(const Data& data)
  : Certificate(data)
{

  auto additionalWire = getSignatureInfo().getCustomTlv(tlv::AdditionalDescription);
  if (additionalWire) {
    ndn::security::AdditionalDescription additional(*additionalWire);
    m_signer = additional.get("signer");
  }

  auto profileWire = getSignatureInfo().getCustomTlv(tlv::Profile);
  if (profileWire) {
    m_profile = Profile(*profileWire);
  }

  auto endorseExtensionBlock = getSignatureInfo().getCustomTlv(tlv::EndorseExtension);
  if (endorseExtensionBlock) {
    EndorseExtension endorseExtension(*endorseExtensionBlock);
    endorseExtension >> m_endorseList;
  }
}

} // namespace chronochat
