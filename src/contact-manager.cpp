/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#if __clang__
#pragma clang diagnostic ignored "-Wtautological-compare"
#endif


#include "contact-manager.h"

#ifndef Q_MOC_RUN
#include <ndn-cpp-dev/face.hpp>
#include <ndn-cpp-dev/security/signature-sha256-with-rsa.hpp>

#ifndef WITH_SECURITY
#include <ndn-cpp-dev/security/validator-null.hpp>
#else
#include <ndn-cpp-dev/security/validator-regex.hpp>
#include <cryptopp/base64.h>
#include <ndn-cpp-dev/security/sec-rule-relative.hpp>
#endif

#include "endorse-collection.pb.h"
#include "logging.h"
#endif

using namespace ndn;
using namespace std;

INIT_LOGGER("ContactManager");

namespace chronos{

ContactManager::ContactManager(shared_ptr<Face> face,
                               QObject* parent)
  : QObject(parent)
  , m_contactStorage(new ContactStorage())
  , m_dnsStorage(new DnsStorage())
  , m_face(face)
{
  initializeSecurity();
}

ContactManager::~ContactManager()
{}

void
ContactManager::initializeSecurity()
{
  
#ifndef WITH_SECURITY
  
  m_keyChain = make_shared<KeyChain>();
  m_validator = make_shared<ValidatorNull>();

#else

  shared_ptr<SecPolicySimple> policy = make_shared<SecPolicySimple>();
  m_verifier = make_shared<Verifier>(policy);
  m_verifier->setFace(m_face);

  policy->addVerificationPolicyRule(make_shared<SecRuleRelative>("^([^<DNS>]*)<DNS><ENDORSED>",
                                                                 "^([^<KEY>]*)<KEY>(<>*)[<ksk-.*><dsk-.*>]<ID-CERT>$",
                                                                 "==", "\\1", "\\1\\2", true));
  policy->addVerificationPolicyRule(make_shared<SecRuleRelative>("^([^<DNS>]*)<DNS><PROFILE>",
                                                                 "^([^<KEY>]*)<KEY>(<>*)[<ksk-.*><dsk-.*>]<ID-CERT>$",
                                                                 "==", "\\1", "\\1\\2", true));
  policy->addVerificationPolicyRule(make_shared<SecRuleRelative>("^([^<PROFILE-CERT>]*)<PROFILE-CERT>",
                                                                 "^([^<KEY>]*)<KEY>(<>*<ksk-.*>)<ID-CERT>$", 
                                                                 "==", "\\1", "\\1\\2", true));
  policy->addVerificationPolicyRule(make_shared<SecRuleRelative>("^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>",
                                                                 "^([^<KEY>]*)<KEY><dsk-.*><ID-CERT>$",
                                                                 ">", "\\1\\2", "\\1", true));
  policy->addVerificationPolicyRule(make_shared<SecRuleRelative>("^([^<KEY>]*)<KEY><dsk-.*><ID-CERT>",
                                                                 "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$",
                                                                 "==", "\\1", "\\1\\2", true));
  policy->addVerificationPolicyRule(make_shared<SecRuleRelative>("^(<>*)$", 
                                                                 "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$", 
                                                                 ">", "\\1", "\\1\\2", true));
  

  policy->addSigningPolicyRule(make_shared<SecRuleRelative>("^([^<DNS>]*)<DNS><PROFILE>",
                                                            "^([^<KEY>]*)<KEY>(<>*)<><ID-CERT>",
                                                            "==", "\\1", "\\1\\2", true));


  const string TrustAnchor("BIICqgOyEIWlKzDI2xX2hdq5Azheu9IVyewcV4uM7ylfh67Y8MIxF3tDCTx5JgEn\
HYMuCaYQm6XuaXTlVfDdWff/K7Xebq8IgGxjNBeU9eMf7Gy9iIMrRAOdBG0dBHmo\
67biGs8F+P1oh1FwKu/FN1AE9vh8HSOJ94PWmjO+6PvITFIXuI3QbcCz8rhvbsfb\
5X/DmfbJ8n8c4X3nVxrBm6fd4z8kOFOvvhgJImvqsow69Uy+38m8gJrmrcWMoPBJ\
WsNLcEriZCt/Dlg7EqqVrIn6ukylKCvVrxA9vm/cEB74J/N+T0JyMRDnTLm17gpq\
Gd75rhj+bLmpOMOBT7Nb27wUKq8gcXzeAADy+p1uZG4A+p1LRVkA+vVrc2stMTM4\
MzMyNTcyMAD6vUlELUNFUlQA+q39PgurHgAAAaID4gKF5vjua9EIr3/Fn8k1AdSc\
nEryjVDW3ikvYoSwjK7egTkAArq1BSc+C6sdAAHiAery+p1uZG4A+p1LRVkA+vVr\
c2stMTM4MzMyNTcyMAD6vUlELUNFUlQAAAAAAAGaFr0wggFjMCIYDzIwMTMxMTAx\
MTcxMTIyWhgPMjAxNDExMDExNzExMjJaMBkwFwYDVQQpExBORE4gVGVzdGJlZCBS\
b290MIIBIDANBgkqhkiG9w0BAQEFAAOCAQ0AMIIBCAKCAQEA06x+elwzWCHa4I3b\
yrYCMAIVxQpRVLuOXp0h+BS+5GNgMVPi7+40o4zSJG+kiU8CIH1mtj8RQAzBX9hF\
I5VAyOC8nS8D8YOfBwt2yRDZPgt1E5PpyYUBiDYuq/zmJDL8xjxAlxrMzVOqD/uj\
/vkkcBM/T1t9Q6p1CpRyq+GMRbV4EAHvH7MFb6bDrH9t8DHEg7NPUCaSQBrd7PvL\
72P+QdiNH9zs/EiVzAkeMG4iniSXLuYM3z0gMqqcyUUUr6r1F9IBmDO+Kp97nZh8\
VCL+cnIEwyzAFAupQH5GoXUWGiee8oKWwH2vGHX7u6sWZsCp15NMSG3OC4jUIZOE\
iVUF1QIBEQAA");

  string decoded;
  CryptoPP::StringSource ss(reinterpret_cast<const unsigned char *>(TrustAnchor.c_str()), 
                            TrustAnchor.size(), 
                            true,
                            new CryptoPP::Base64Decoder(new CryptoPP::StringSink(decoded)));
  Data data;
  data.wireDecode(Block(reinterpret_cast<const uint8_t*>(decoded.c_str()), decoded.size()));
  shared_ptr<IdentityCertificate> anchor = make_shared<IdentityCertificate>(data);
  policy->addTrustAnchor(anchor);  
#endif
}


void
ContactManager::fetchSelfEndorseCertificate(const ndn::Name& identity)
{
  Name interestName = identity;
  interestName.append("DNS").append("PROFILE");
  
  Interest interest(interestName);
  interest.setMustBeFresh(true);

  OnDataValidated onValidated = bind(&ContactManager::onDnsSelfEndorseCertValidated, this, _1, identity);
  OnDataValidationFailed onValidationFailed = bind(&ContactManager::onDnsSelfEndorseCertValidationFailed, this, _1, identity);
  TimeoutNotify timeoutNotify = bind(&ContactManager::onDnsSelfEndorseCertTimeoutNotify, this, identity);

  sendInterest(interest, onValidated, onValidationFailed, timeoutNotify);
}

void
ContactManager::onDnsSelfEndorseCertValidated(const shared_ptr<const Data>& data, 
                                              const Name& identity)
{
  try{
    Data plainData;
    plainData.wireDecode(data->getContent().blockFromValue());
    EndorseCertificate selfEndorseCertificate(plainData);
    if(Validator::verifySignature(plainData, plainData.getSignature(), selfEndorseCertificate.getPublicKeyInfo()))
      emit contactFetched(selfEndorseCertificate); 
    else
      emit contactFetchFailed(identity);
  }catch(...){
    emit contactFetchFailed (identity);
  }
}

void
ContactManager::fetchCollectEndorse(const Name& identity)
{
  Name interestName = identity;
  interestName.append("DNS").append("ENDORSED");

  Interest interest(interestName);
  interest.setInterestLifetime(1000);
  interest.setMustBeFresh(true);

  OnDataValidated onValidated = bind(&ContactManager::onDnsCollectEndorseValidated, this, _1, identity);
  OnDataValidationFailed onValidationFailed = bind(&ContactManager::onDnsCollectEndorseValidationFailed, this, _1, identity);
  TimeoutNotify timeoutNotify = bind(&ContactManager::onDnsCollectEndorseTimeoutNotify, this, identity);

  sendInterest(interest, onValidated, onValidationFailed, timeoutNotify);
}

void
ContactManager::fetchKey(const Name& certName)
{
  Name interestName = certName;
  
  Interest interest(interestName);
  interest.setInterestLifetime(1000);
  interest.setMustBeFresh(true);

  OnDataValidated onValidated = bind(&ContactManager::onKeyValidated, this, _1, certName);
  OnDataValidationFailed onValidationFailed = bind(&ContactManager::onKeyValidationFailed, this, _1, certName);
  TimeoutNotify timeoutNotify = bind(&ContactManager::onKeyTimeoutNotify, this, certName);

  sendInterest(interest, onValidated, onValidationFailed, timeoutNotify);
}

void
ContactManager::onKeyValidated(const shared_ptr<const Data>& data, const Name& identity)
{
  IdentityCertificate identityCertificate(*data);
  Profile profile(identityCertificate);

  try{
    EndorseCertificate endorseCertificate(identityCertificate, profile);
    m_keyChain->sign(endorseCertificate);
    emit contactKeyFetched (endorseCertificate); 
  }catch(...){
    return;
  }
}

void
ContactManager::fetchIdCertificate(const Name& certName)
{
  Name interestName = certName;
  
  Interest interest(interestName);
  interest.setInterestLifetime(1000);
  interest.setMustBeFresh(true);

  OnDataValidated onValidated = bind(&ContactManager::onIdCertValidated, this, _1, certName);
  OnDataValidationFailed onValidationFailed = bind(&ContactManager::onIdCertValidationFailed, this, _1, certName);
  TimeoutNotify timeoutNotify = bind(&ContactManager::onIdCertTimeoutNotify, this, certName);

  sendInterest(interest, onValidated, onValidationFailed, timeoutNotify);
}

void
ContactManager::updateProfileData(const Name& identity)
{
  // Get current profile;
  shared_ptr<Profile> newProfile = m_contactStorage->getSelfProfile(identity);
  if(!static_cast<bool>(newProfile))
    return;

  shared_ptr<EndorseCertificate> newEndorseCertificate = getSignedSelfEndorseCertificate(identity, *newProfile);

  if(!static_cast<bool>(newEndorseCertificate))
    return;

  m_contactStorage->addSelfEndorseCertificate(*newEndorseCertificate, identity);

  publishSelfEndorseCertificateInDNS(*newEndorseCertificate);
}

void
ContactManager::updateEndorseCertificate(const ndn::Name& identity, const ndn::Name& signerIdentity)
{
  shared_ptr<EndorseCertificate> newEndorseCertificate = generateEndorseCertificate(identity, signerIdentity);

  if(!static_cast<bool>(newEndorseCertificate))
    return;

  m_contactStorage->addEndorseCertificate(*newEndorseCertificate, identity);

  publishEndorseCertificateInDNS(*newEndorseCertificate, signerIdentity);
}

shared_ptr<EndorseCertificate> 
ContactManager::generateEndorseCertificate(const Name& identity, const Name& signerIdentity)
{
  shared_ptr<ContactItem> contact = getContact(identity);
  if(!static_cast<bool>(contact))
    return shared_ptr<EndorseCertificate>();

  Name signerKeyName = m_keyChain->getDefaultKeyNameForIdentity(signerIdentity);

  vector<string> endorseList;
  m_contactStorage->getEndorseList(identity, endorseList);

  try{
    shared_ptr<EndorseCertificate> cert = make_shared<EndorseCertificate>(contact->getSelfEndorseCertificate(), signerKeyName, endorseList); 
    m_keyChain->signByIdentity(*cert, signerIdentity);
    return cert;
  }catch(...){
    return shared_ptr<EndorseCertificate>();
  } 
}

shared_ptr<EndorseCertificate>
ContactManager::getSignedSelfEndorseCertificate(const Name& identity,
                                                const Profile& profile)
{
  Name certificateName = m_keyChain->getDefaultCertificateNameForIdentity(identity);
  if(0 == certificateName.size())
    return shared_ptr<EndorseCertificate>();

  Name signingKeyName = IdentityCertificate::certificateNameToPublicKeyName(certificateName);
  shared_ptr<IdentityCertificate> kskCert;

  if(signingKeyName.get(-1).toEscapedString().substr(0,4) == "dsk-")
    {
      shared_ptr<IdentityCertificate> signingCert = m_keyChain->getCertificate(certificateName);
      if(!static_cast<bool>(signingCert))
        return shared_ptr<EndorseCertificate>();

      try{
        SignatureSha256WithRsa dskCertSig(signingCert->getSignature());
        Name keyName = IdentityCertificate::certificateNameToPublicKeyName(dskCertSig.getKeyLocator().getName());
        Name kskCertName = m_keyChain->getDefaultCertificateNameForKey(keyName);
        kskCert = m_keyChain->getCertificate(kskCertName);
      }catch(...){
        return shared_ptr<EndorseCertificate>();
      }
    }      
  else
    kskCert = m_keyChain->getCertificate(certificateName);

  if(!static_cast<bool>(kskCert))
    return shared_ptr<EndorseCertificate>();

  vector<string> endorseList;
  Profile::const_iterator it = profile.begin();
  for(; it != profile.end(); it++)
    endorseList.push_back(it->first);
  
  try{
    shared_ptr<EndorseCertificate> selfEndorseCertificate = make_shared<EndorseCertificate>(*kskCert, profile, endorseList);
    m_keyChain->sign(*selfEndorseCertificate, kskCert->getName());
    return selfEndorseCertificate;
  }catch(...){
    return shared_ptr<EndorseCertificate>();
  } 
}

void
ContactManager::publishSelfEndorseCertificateInDNS(const EndorseCertificate& selfEndorseCertificate)
{
  Data data;

  Name identity = selfEndorseCertificate.getPublicKeyName().getPrefix(-1);

  Name dnsName = identity;
  dnsName.append("DNS").append("PROFILE").appendVersion();
  data.setName(dnsName);
  data.setContent(selfEndorseCertificate.wireEncode());

  m_keyChain->signByIdentity(data, identity);
  m_dnsStorage->updateDnsSelfProfileData(data, identity);
  m_face->put(data);
}

void
ContactManager::publishEndorseCertificateInDNS(const EndorseCertificate& endorseCertificate, const Name& signerIdentity)
{
  Data data;

  Name endorsee = endorseCertificate.getPublicKeyName().getPrefix(-1);

  Name dnsName = signerIdentity;
  dnsName.append("DNS").append(endorsee.wireEncode()).append("ENDORSEE").appendVersion();
  data.setName(dnsName);

  data.setContent(endorseCertificate.wireEncode());

  m_keyChain->signByIdentity(data, signerIdentity);
  m_dnsStorage->updateDnsEndorseOthers(data, signerIdentity, endorsee);
  m_face->put(data);
}

void
ContactManager::publishCollectEndorsedDataInDNS(const Name& identity)
{
  Data data;

  Name dnsName = identity;
  dnsName.append("DNS").append("ENDORSED").appendVersion();
  data.setName(dnsName);
  
  vector<Buffer> collectEndorseList;
  m_contactStorage->getCollectEndorseList(identity, collectEndorseList);
  
  Chronos::EndorseCollection endorseCollection;

  vector<Buffer>::const_iterator it = collectEndorseList.begin();
  for(; it != collectEndorseList.end(); it++)
    {
      string entryStr(reinterpret_cast<const char*>(it->buf()), it->size());
      endorseCollection.add_endorsement()->set_blob(entryStr);
    }

  string encoded;
  endorseCollection.SerializeToString(&encoded);

  data.setContent(reinterpret_cast<const uint8_t*>(encoded.c_str()), encoded.size());
  
  m_keyChain->signByIdentity(data, identity);
  m_dnsStorage->updateDnsOthersEndorse(data, identity);
  m_face->put(data);
}

void
ContactManager::addContact(const IdentityCertificate& identityCertificate, const Profile& profile)
{
  try{
    EndorseCertificate endorseCertificate(identityCertificate, profile);
    
    m_keyChain->signByIdentity(endorseCertificate, m_defaultIdentity);

    ContactItem contactItem(endorseCertificate);

    m_contactStorage->addContact(contactItem);

    emit contactAdded(contactItem.getNameSpace());

  }catch(std::runtime_error& e){
    emit warning(e.what());
    _LOG_ERROR("Exception: " << e.what());
    return;
  }
}

void
ContactManager::removeContact(const Name& contactNameSpace)
{
  shared_ptr<ContactItem> contact = getContact(contactNameSpace);
  if(!static_cast<bool>(contact))
    return;
  m_contactStorage->removeContact(contactNameSpace);
  emit contactRemoved(contact->getPublicKeyName());
}

}//chronos


#if WAF
#include "contact-manager.moc"
#include "contact-manager.cpp.moc"
#endif
