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
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreorder"
#pragma clang diagnostic ignored "-Wtautological-compare"
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-function"
#elif __GNUC__
#pragma GCC diagnostic ignored "-Wreorder"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif


#include "contact-manager.h"

#ifndef Q_MOC_RUN
#include <ndn-cpp/face.hpp>
#include <ndn-cpp/security/signature-sha256-with-rsa.hpp>
#include <ndn-cpp/security/verifier.hpp>
#include <cryptopp/base64.h>
#include <ndn-cpp-et/policy/sec-rule-identity.hpp>
#include <ndn-cpp-et/policy/sec-policy-simple.hpp>
#include <fstream>
#include "endorse-collection.pb.h"
#include "logging.h"
#endif

using namespace ndn;
using namespace ndn::ptr_lib;
using namespace std;

INIT_LOGGER("ContactManager");

ContactManager::ContactManager(shared_ptr<KeyChain> keyChain, 
                               shared_ptr<Face> face,
                               QObject* parent)
  : QObject(parent),
    m_face(face)
{
  m_keyChain = keyChain;
  m_contactStorage = make_shared<ContactStorage>();
  m_dnsStorage = make_shared<DnsStorage>();

  initializeSecurity();
}

ContactManager::~ContactManager()
{}

void
ContactManager::initializeSecurity()
{
  shared_ptr<SecPolicySimple> policy = make_shared<SecPolicySimple>();
  m_verifier = make_shared<Verifier>(policy);
  m_verifier->setFace(m_face);

  policy->addVerificationPolicyRule(make_shared<SecRuleIdentity>("^([^<DNS>]*)<DNS><ENDORSED>",
                                                                 "^([^<KEY>]*)<KEY>(<>*)[<ksk-.*><dsk-.*>]<ID-CERT>$",
                                                                 "==", "\\1", "\\1\\2", true));
  policy->addVerificationPolicyRule(make_shared<SecRuleIdentity>("^([^<DNS>]*)<DNS><PROFILE>",
                                                                 "^([^<KEY>]*)<KEY>(<>*)[<ksk-.*><dsk-.*>]<ID-CERT>$",
                                                                 "==", "\\1", "\\1\\2", true));
  policy->addVerificationPolicyRule(make_shared<SecRuleIdentity>("^([^<PROFILE-CERT>]*)<PROFILE-CERT>",
                                                                 "^([^<KEY>]*)<KEY>(<>*<ksk-.*>)<ID-CERT>$", 
                                                                 "==", "\\1", "\\1\\2", true));
  policy->addVerificationPolicyRule(make_shared<SecRuleIdentity>("^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>",
                                                                 "^([^<KEY>]*)<KEY><dsk-.*><ID-CERT>$",
                                                                 ">", "\\1\\2", "\\1", true));
  policy->addVerificationPolicyRule(make_shared<SecRuleIdentity>("^([^<KEY>]*)<KEY><dsk-.*><ID-CERT>",
                                                                 "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$",
                                                                 "==", "\\1", "\\1\\2", true));
  policy->addVerificationPolicyRule(make_shared<SecRuleIdentity>("^(<>*)$", 
                                                                 "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$", 
                                                                 ">", "\\1", "\\1\\2", true));
  

  policy->addSigningPolicyRule(make_shared<SecRuleIdentity>("^([^<DNS>]*)<DNS><PROFILE>",
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

#ifdef _DEBUG

  const string FakeAnchor("BIICqgOyEIVAaoHnQZIx5osAuY2fKte4HBSrxyam7MY6/kp+w47O1bGdd2KjeZKV\
zZzQd3EQorDC3KUPbB6ql30jYfspvo4OPSlIuDrkyROaoZ+MSKyzQYpB6CZcTjBa\
qcWYFOfwUlcWvkbd00X4bkc5PkcWpVdRrx+NCTiq9EXes//hOHpEJHMNsJUi45O+\
6M4OE6/sNEqs/ryHn2w1vCqwPpG8xzcd0prQUdCH2MGE77F+H0XFDuWp8mrT37Uw\
DUy7Ltm+7nDTHSQy2J3Zk4Q+0tjxCzSw4owEpwOHr+afdkuE3v9aB2NRQBBDCEmL\
Ykz4sYX3XE8MVFqRn1HHWCkszjDg+F0UAADy+p1uZG4A+p1LRVkA+vVrc2stMTM4\
MjkzNDE5OAD6vUlELUNFUlQA+s39/////95rc7MAAAGiA+IChaK1eVvzlkg6BJAw\
qiOpxRoezQ0hAHOBbPRLeBllxMN7AAK6tQUm3mtztQAB4gHq8vqdbmRuAPqdS0VZ\
APr1a3NrLTEzODI5MzQxOTgA+r1JRC1DRVJUAAAAAAABmhblMIIBaDAiGA8yMDEz\
MTAyODAwMDAwMFoYDzIwMzMxMDI4MDAwMDAwWjAcMBoGA1UEKRMTL25kbi9rc2st\
MTM4MjkzNDE5ODCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAK2htIFF\
/PH+SJsGOA6jhpFT74xfLJlgZNJOnKzl27HI2gupE0mainWj/HqVzdGxD6jOOReI\
sul+eQyEyBYq4e35pLmdJGlux/+UPQ51DD8jg04GrUPewV7+iGm6usp/7xEGHbah\
H2Grv/bsGrt6aRA8cKmdIc+rehxZCVFtiwSEHTnOWzn3lfZR5xnjF9aGX+uGo1hA\
gMwu1ECxg4H3O4z1tbTzji5+WH0RDsPRlgzQX6wAQH8btlQyoFJfljEA3QaOtDaB\
OcfegIlClzutmgJnK9i5ZLz2Mjvx49dlCWAVKg65vOXMLC/33jD9F+V8urwsBlOb\
F7Wh5ayeo8NBKDsCAwEAAQAA");

  string decoded2;
  CryptoPP::StringSource ss2(reinterpret_cast<const unsigned char *>(FakeAnchor.c_str()), 
                            FakeAnchor.size(), 
                            true,
                            new CryptoPP::Base64Decoder(new CryptoPP::StringSink(decoded2)));
  Data data2;
  data2.wireDecode(Block(reinterpret_cast<const uint8_t*>(decoded.c_str()), decoded.size()));
  shared_ptr<IdentityCertificate>anchor2 = make_shared<IdentityCertificate>(data2);
  policy->addTrustAnchor(anchor2);  

#endif
}


void
ContactManager::fetchSelfEndorseCertificate(const ndn::Name& identity)
{
  Name interestName = identity;
  interestName.append("DNS").append("PROFILE");
  
  Interest interest(interestName);

  OnVerified onVerified = func_lib::bind(&ContactManager::onDnsSelfEndorseCertificateVerified, this, _1, identity);
  OnVerifyFailed onVerifyFailed = func_lib::bind(&ContactManager::onDnsSelfEndorseCertificateVerifyFailed, this, _1, identity);
  TimeoutNotify timeoutNotify = func_lib::bind(&ContactManager::onDnsSelfEndorseCertificateTimeoutNotify, this, identity);

  sendInterest(interest, onVerified, onVerifyFailed, timeoutNotify);
}

void
ContactManager::onDnsSelfEndorseCertificateTimeoutNotify(const Name& identity)
{ emit contactFetchFailed(identity); }

void
ContactManager::onDnsSelfEndorseCertificateVerified(const shared_ptr<Data>& data, 
                                                    const Name& identity)
{
  try{
    Data plainData;
    plainData.wireDecode(Block(data->getContent().value(), data->getContent().value_size()));
    EndorseCertificate selfEndorseCertificate(plainData);
    if(Verifier::verifySignature(plainData, plainData.getSignature(), selfEndorseCertificate.getPublicKeyInfo()))
      emit contactFetched (selfEndorseCertificate); 
    else
      emit contactFetchFailed (identity);
  }catch(std::exception& e){
    _LOG_ERROR("Exception: " << e.what());
    emit contactFetchFailed (identity);
  }
}

void
ContactManager::onDnsSelfEndorseCertificateVerifyFailed(const shared_ptr<Data>& data, 
                                                        const Name& identity)
{ emit contactFetchFailed (identity); }

void
ContactManager::fetchCollectEndorse(const ndn::Name& identity)
{
  Name interestName = identity;
  interestName.append("DNS").append("ENDORSED");

  Interest interest(interestName);
  interest.setInterestLifetime(1000);

  OnVerified onVerified = func_lib::bind(&ContactManager::onDnsCollectEndorseVerified, this, _1, identity);
  OnVerifyFailed onVerifyFailed = func_lib::bind(&ContactManager::onDnsCollectEndorseVerifyFailed, this, _1, identity);
  TimeoutNotify timeoutNotify = func_lib::bind(&ContactManager::onDnsCollectEndorseTimeoutNotify, this, identity);

  sendInterest(interest, onVerified, onVerifyFailed, timeoutNotify);
}

void
ContactManager::onDnsCollectEndorseTimeoutNotify(const Name& identity)
{
  emit collectEndorseFetchFailed (identity);
}

void
ContactManager::onDnsCollectEndorseVerified(const shared_ptr<Data>& data, const Name& identity)
{ emit collectEndorseFetched (*data); }

void
ContactManager::onDnsCollectEndorseVerifyFailed(const shared_ptr<Data>& data, const Name& identity)
{ emit collectEndorseFetchFailed (identity); }


void
ContactManager::fetchKey(const ndn::Name& certName)
{
  Name interestName = certName;
  
  Interest interest(interestName);
  interest.setInterestLifetime(1000);

  OnVerified onVerified = func_lib::bind(&ContactManager::onKeyVerified, this, _1, certName);
  OnVerifyFailed onVerifyFailed = func_lib::bind(&ContactManager::onKeyVerifyFailed, this, _1, certName);
  TimeoutNotify timeoutNotify = func_lib::bind(&ContactManager::onKeyTimeoutNotify, this, certName);

  sendInterest(interest, onVerified, onVerifyFailed, timeoutNotify);
}


void
ContactManager::onKeyVerified(const shared_ptr<Data>& data, const Name& identity)
{
  IdentityCertificate identityCertificate(*data);

  Profile profile(identityCertificate);
  ProfileData profileData(profile);

  Name certificateName = m_keyChain->getDefaultCertificateName();
  m_keyChain->sign(profileData, certificateName);

  try{
    EndorseCertificate endorseCertificate(identityCertificate, profileData);
    m_keyChain->sign(endorseCertificate, certificateName);
    emit contactKeyFetched (endorseCertificate); 
  }catch(std::exception& e){
    _LOG_ERROR("Exception: " << e.what());
    return;
  }
}

void
ContactManager::onKeyVerifyFailed(const shared_ptr<Data>& data, const Name& identity)
{ 
  _LOG_DEBUG("Key cannot be verified!");
  emit contactKeyFetchFailed (identity); 
}

void
ContactManager::onKeyTimeoutNotify(const Name& identity)
{ 
  _LOG_DEBUG("Key timeout!");
  emit contactKeyFetchFailed(identity); 
}

void
ContactManager::fetchIdCertificate(const Name& certName)
{
  Name interestName = certName;
  
  Interest interest(interestName);
  interest.setInterestLifetime(1000);

  OnVerified onVerified = boost::bind(&ContactManager::onIdCertificateVerified, this, _1, certName);
  OnVerifyFailed onVerifyFailed = boost::bind(&ContactManager::onIdCertificateVerifyFailed, this, _1, certName);
  TimeoutNotify timeoutNotify = boost::bind(&ContactManager::onIdCertificateTimeoutNotify, this, certName);

  sendInterest(interest, onVerified, onVerifyFailed, timeoutNotify);
}

void
ContactManager::onIdCertificateTimeoutNotify(const Name& identity)
{ 
  emit contactCertificateFetchFailed (identity); 
}


void
ContactManager::onIdCertificateVerified(const shared_ptr<Data>& data, const Name& identity)
{
  IdentityCertificate identityCertificate(*data);
  emit contactCertificateFetched(identityCertificate);
}

void
ContactManager::onIdCertificateVerifyFailed(const shared_ptr<Data>& data, const Name& identity)
{ 
  emit contactCertificateFetchFailed (identity); 
}

void
ContactManager::onTargetData(const shared_ptr<const ndn::Interest>& interest, 
                             const shared_ptr<Data>& data,
                             const OnVerified& onVerified,
                             const OnVerifyFailed& onVerifyFailed)
{
  m_verifier->verifyData(data, onVerified, onVerifyFailed);
}

void
ContactManager::onTargetTimeout(const shared_ptr<const ndn::Interest>& interest, 
                                int retry,
                                const OnVerified& onVerified,
                                const OnVerifyFailed& onVerifyFailed,
                                const TimeoutNotify& timeoutNotify)
{
  if(retry > 0)
    sendInterest(*interest, onVerified, onVerifyFailed, timeoutNotify, retry-1);
  else
    {
      _LOG_DEBUG("Interest: " << interest->getName().toUri() << " eventually times out!");
      timeoutNotify();
    }
}

void
ContactManager::sendInterest(const Interest& interest,
                             const OnVerified& onVerified,
                             const OnVerifyFailed& onVerifyFailed,
                             const TimeoutNotify& timeoutNotify,
                             int retry /* = 1 */)
{
  uint64_t id = m_face->expressInterest(interest, 
                          boost::bind(&ContactManager::onTargetData, 
                                      this,
                                      _1,
                                      _2,
                                      onVerified, 
                                      onVerifyFailed),
                          boost::bind(&ContactManager::onTargetTimeout,
                                      this,
                                      _1,
                                      retry,
                                      onVerified,
                                      onVerifyFailed,
                                      timeoutNotify));

  // _LOG_DEBUG("id: " << id << " entry id: " << m_face->getNode().getEntryIndexForExpressedInterest(interest.getName()));
}

void
ContactManager::updateProfileData(const Name& identity)
{
  // Get current profile;
  shared_ptr<Profile> newProfile = m_contactStorage->getSelfProfile(identity);
  if(static_cast<bool>(newProfile))
    return;

  shared_ptr<EndorseCertificate> newEndorseCertificate = getSignedSelfEndorseCertificate(identity, *newProfile);

  if(static_cast<bool>(newEndorseCertificate))
    return;

  // Check if profile exists
  try{
    Block profileDataBlob = m_contactStorage->getSelfEndorseCertificate(identity);
    m_contactStorage->updateSelfEndorseCertificate(*newEndorseCertificate, identity);
  }catch(ContactStorage::Error &e){
    m_contactStorage->addSelfEndorseCertificate(*newEndorseCertificate, identity);
  }

  publishSelfEndorseCertificateInDNS(*newEndorseCertificate);
}

void
ContactManager::updateEndorseCertificate(const ndn::Name& identity, const ndn::Name& signerIdentity)
{
  shared_ptr<EndorseCertificate> newEndorseCertificate = generateEndorseCertificate(identity, signerIdentity);

  if(static_cast<bool>(newEndorseCertificate))
    return;

  try{
    Block oldEndorseCertificateBlob = m_contactStorage->getEndorseCertificate(identity);
    m_contactStorage->updateEndorseCertificate(*newEndorseCertificate, identity);
  }catch(ContactStorage::Error &e){
    m_contactStorage->addEndorseCertificate(*newEndorseCertificate, identity);
  }

  publishEndorseCertificateInDNS(*newEndorseCertificate, signerIdentity);
}

shared_ptr<EndorseCertificate> 
ContactManager::generateEndorseCertificate(const Name& identity, const Name& signerIdentity)
{
  shared_ptr<ContactItem> contact = getContact(identity);
  if(static_cast<bool>(contact))
    return shared_ptr<EndorseCertificate>();

  Name signerKeyName = m_keyChain->getDefaultKeyNameForIdentity(signerIdentity);
  Name signerCertName = m_keyChain->getDefaultCertificateNameForIdentity(signerIdentity);

  vector<string> endorseList;
  m_contactStorage->getEndorseList(identity, endorseList);

  
  try{
    shared_ptr<EndorseCertificate> cert = make_shared<EndorseCertificate>(contact->getSelfEndorseCertificate(), signerKeyName, endorseList); 
    m_keyChain->sign(*cert, signerCertName);
    return cert;
  }catch(std::exception& e){
    _LOG_ERROR("Exception: " << e.what());
    return shared_ptr<EndorseCertificate>();
  } 
}

void
ContactManager::getContactItemList(vector<shared_ptr<ContactItem> >& contacts)
{ return m_contactStorage->getAllContacts(contacts); }

shared_ptr<ContactItem>
ContactManager::getContact(const ndn::Name& contactNamespace)
{ return m_contactStorage->getContact(contactNamespace); }

shared_ptr<EndorseCertificate>
ContactManager::getSignedSelfEndorseCertificate(const Name& identity,
                                                const Profile& profile)
{
  Name certificateName = m_keyChain->getDefaultCertificateNameForIdentity(identity);
  if(0 == certificateName.size())
    return shared_ptr<EndorseCertificate>();

  ProfileData profileData(profile);
  m_keyChain->sign(profileData, certificateName);

  shared_ptr<IdentityCertificate> signingCert = m_keyChain->getCertificate(certificateName);
  if(static_cast<bool>(signingCert))
    return shared_ptr<EndorseCertificate>();

  Name signingKeyName = IdentityCertificate::certificateNameToPublicKeyName(signingCert->getName());

  shared_ptr<IdentityCertificate> kskCert;
  if(signingKeyName.get(-1).toEscapedString().substr(0,4) == string("dsk-"))
    {
      SignatureSha256WithRsa dskCertSig(signingCert->getSignature());
      // HACK! KSK certificate should be retrieved from network.
      Name keyName = IdentityCertificate::certificateNameToPublicKeyName(dskCertSig.getKeyLocator().getName());

      // TODO: check null existing cases.
      Name kskCertName = m_keyChain->getDefaultCertificateNameForIdentity(keyName.getPrefix(-1));

      kskCert = m_keyChain->getCertificate(kskCertName);
    }
  else
    {
      kskCert = signingCert;
    }

  if(static_cast<bool>(kskCert))
    return shared_ptr<EndorseCertificate>();

  vector<string> endorseList;
  Profile::const_iterator it = profile.begin();
  for(; it != profile.end(); it++)
    endorseList.push_back(it->first);
  
  try{
    shared_ptr<EndorseCertificate> selfEndorseCertificate = make_shared<EndorseCertificate>(*kskCert, profileData, endorseList);
    m_keyChain->sign(*selfEndorseCertificate, kskCert->getName());

    return selfEndorseCertificate;
  }catch(std::exception& e){
    _LOG_ERROR("Exception: " << e.what());
    return shared_ptr<EndorseCertificate>();
  } 
}


void
ContactManager::publishSelfEndorseCertificateInDNS(const EndorseCertificate& selfEndorseCertificate)
{
  Data data;

  Name keyName = selfEndorseCertificate.getPublicKeyName();
  Name identity = keyName.getSubName(0, keyName.size()-1);

  time_t nowSeconds = time(NULL);
  struct tm current = *gmtime(&nowSeconds);
  MillisecondsSince1970 version = timegm(&current) * 1000.0;

  Name dnsName = identity;
  dnsName.append("DNS").append("PROFILE").appendVersion(version);
  data.setName(dnsName);

  data.setContent(selfEndorseCertificate.wireEncode());

  Name signCertName = m_keyChain->getDefaultCertificateNameForIdentity(identity);
  m_keyChain->sign(data, signCertName);

  m_dnsStorage->updateDnsSelfProfileData(data, identity);
  
  m_face->put(data);
}

void
ContactManager::publishEndorseCertificateInDNS(const EndorseCertificate& endorseCertificate, const Name& signerIdentity)
{
  Data data;

  Name keyName = endorseCertificate.getPublicKeyName();
  Name endorsee = keyName.getSubName(0, keyName.size()-1);

  time_t nowSeconds = time(NULL);
  struct tm current = *gmtime(&nowSeconds);
  MillisecondsSince1970 version = timegm(&current) * 1000.0;

  Name dnsName = signerIdentity;
  dnsName.append("DNS").append(endorsee).append("ENDORSEE").appendVersion(version);
  data.setName(dnsName);

  data.setContent(endorseCertificate.wireEncode());

  Name signCertName = m_keyChain->getDefaultCertificateNameForIdentity(signerIdentity);
  m_keyChain->sign(data, signCertName);

  m_dnsStorage->updateDnsEndorseOthers(data, signerIdentity, endorsee);

  m_face->put(data);
}

void
ContactManager::publishEndorsedDataInDns(const Name& identity)
{
  Data data;

  time_t nowSeconds = time(NULL);
  struct tm current = *gmtime(&nowSeconds);
  MillisecondsSince1970 version = timegm(&current) * 1000.0;

  Name dnsName = identity;
  dnsName.append("DNS").append("ENDORSED").appendVersion(version);
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
  
  Name signCertName = m_keyChain->getDefaultCertificateNameForIdentity(identity);
  m_keyChain->sign(data, signCertName);

  m_dnsStorage->updateDnsOthersEndorse(data, identity);

  m_face->put(data);
}

void
ContactManager::addContact(const IdentityCertificate& identityCertificate, const Profile& profile)
{
  ProfileData profileData(profile);
  
  Name certificateName = m_keyChain->getDefaultCertificateNameForIdentity (m_defaultIdentity);
  m_keyChain->sign(profileData, certificateName);


  try{
    EndorseCertificate endorseCertificate(identityCertificate, profileData);
    
    m_keyChain->sign(endorseCertificate, certificateName);

    ContactItem contactItem(endorseCertificate);

    m_contactStorage->addContact(contactItem);

    emit contactAdded(contactItem.getNameSpace());

  }catch(std::exception& e){
    emit warning(e.what());
    _LOG_ERROR("Exception: " << e.what());
    return;
  }
}

void
ContactManager::removeContact(const ndn::Name& contactNameSpace)
{
  shared_ptr<ContactItem> contact = getContact(contactNameSpace);
  if(static_cast<bool>(contact))
    return;
  m_contactStorage->removeContact(contactNameSpace);
  emit contactRemoved(contact->getPublicKeyName());
}


#if WAF
#include "contact-manager.moc"
#include "contact-manager.cpp.moc"
#endif
