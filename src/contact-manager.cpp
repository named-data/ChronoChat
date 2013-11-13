/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "contact-manager.h"

#ifndef Q_MOC_RUN
#include <ndn.cxx/wrapper/wrapper.h>
#include <ndn.cxx/security/keychain.h>
#include <ndn.cxx/security/policy/simple-policy-manager.h>
#include <ndn.cxx/security/policy/identity-policy-rule.h>
#include <ndn.cxx/helpers/der/der.h>
#include <cryptopp/base64.h>
#include <fstream>
#include "logging.h"
#endif

using namespace ndn;
using namespace ndn::security;

INIT_LOGGER("ContactManager");

ContactManager::ContactManager(QObject* parent)
  : QObject(parent)
{
  m_contactStorage = Ptr<ContactStorage>::Create();
  m_dnsStorage = Ptr<DnsStorage>::Create();

  setKeychain();
}

ContactManager::~ContactManager()
{
}

void
ContactManager::setWrapper()
{
  try{
    m_wrapper = Ptr<Wrapper>(new Wrapper(m_keychain));
  }catch(ndn::Error::ndnOperation& e){
    emit noNdnConnection(QString::fromStdString("Cannot conect to ndnd!\nHave you started your ndnd?"));
  }
}

void
ContactManager::setKeychain()
{
  Ptr<IdentityManager> identityManager = Ptr<IdentityManager>::Create();
  Ptr<SimplePolicyManager> policyManager = Ptr<SimplePolicyManager>::Create();

  Ptr<Keychain> keychain = Ptr<Keychain>(new Keychain(identityManager, policyManager, NULL));

  policyManager->addVerificationPolicyRule(Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<DNS>]*)<DNS><ENDORSED>",
                                                                                          "^([^<KEY>]*)<KEY>(<>*)[<ksk-.*><dsk-.*>]<ID-CERT>$",
                                                                                          "==", "\\1", "\\1\\2", true)));
  policyManager->addVerificationPolicyRule(Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<DNS>]*)<DNS><PROFILE>",
                                                                                          "^([^<KEY>]*)<KEY>(<>*)[<ksk-.*><dsk-.*>]<ID-CERT>$",
                                                                                          "==", "\\1", "\\1\\2", true)));
  policyManager->addVerificationPolicyRule(Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<PROFILE-CERT>]*)<PROFILE-CERT>",
											  "^([^<KEY>]*)<KEY>(<>*<ksk-.*>)<ID-CERT>$", 
											  "==", "\\1", "\\1\\2", true)));
  policyManager->addVerificationPolicyRule(Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>",
											  "^([^<KEY>]*)<KEY><dsk-.*><ID-CERT>$",
											  ">", "\\1\\2", "\\1", true)));
  policyManager->addVerificationPolicyRule(Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<KEY>]*)<KEY><dsk-.*><ID-CERT>",
											  "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$",
											  "==", "\\1", "\\1\\2", true)));
  policyManager->addVerificationPolicyRule(Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^(<>*)$", 
                                                                                          "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$", 
                                                                                          ">", "\\1", "\\1\\2", true)));
  

  policyManager->addSigningPolicyRule(Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<DNS>]*)<DNS><PROFILE>",
                                                                                     "^([^<KEY>]*)<KEY>(<>*)<><ID-CERT>",
                                                                                     "==", "\\1", "\\1\\2", true)));


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
  Ptr<Blob> blob = Ptr<Blob>(new Blob(decoded.c_str(), decoded.size()));
  Ptr<Data> data = Data::decodeFromWire(blob);
  Ptr<IdentityCertificate>anchor = Ptr<IdentityCertificate>(new IdentityCertificate(*data));
  policyManager->addTrustAnchor(anchor);  

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
  Ptr<Blob> blob2 = Ptr<Blob>(new Blob(decoded2.c_str(), decoded2.size()));
  Ptr<Data> data2 = Data::decodeFromWire(blob2);
  Ptr<IdentityCertificate>anchor2 = Ptr<IdentityCertificate>(new IdentityCertificate(*data2));
  policyManager->addTrustAnchor(anchor2);  

#endif

  m_keychain = keychain;
}


void
ContactManager::fetchSelfEndorseCertificate(const ndn::Name& identity)
{
  Name interestName = identity;
  interestName.append("DNS").append("PROFILE");
  
  Ptr<Interest> interestPtr = Ptr<Interest>(new Interest(interestName));
  interestPtr->setChildSelector(Interest::CHILD_RIGHT);
  Ptr<Closure> closure = Ptr<Closure> (new Closure(boost::bind(&ContactManager::onDnsSelfEndorseCertificateVerified, 
                                                               this,
                                                               _1,
                                                               identity),
						   boost::bind(&ContactManager::onDnsSelfEndorseCertificateTimeout,
                                                               this,
                                                               _1, 
                                                               _2,
                                                               identity,
                                                               0),
						   boost::bind(&ContactManager::onDnsSelfEndorseCertificateUnverified,
                                                               this,
                                                               _1,
                                                               identity)));
  m_wrapper->sendInterest(interestPtr, closure);
}

void
ContactManager::fetchCollectEndorse(const ndn::Name& identity)
{
  Name interestName = identity;
  interestName.append("DNS").append("ENDORSED");

  Ptr<Interest> interestPtr = Ptr<Interest>(new Interest(interestName));
  interestPtr->setChildSelector(Interest::CHILD_RIGHT);
  interestPtr->setInterestLifetime(1);
  Ptr<Closure> closure = Ptr<Closure> (new Closure(boost::bind(&ContactManager::onDnsCollectEndorseVerified, 
                                                               this,
                                                               _1,
                                                               identity),
						   boost::bind(&ContactManager::onDnsCollectEndorseTimeout,
                                                               this,
                                                               _1, 
                                                               _2,
                                                               identity,
                                                               0),
						   boost::bind(&ContactManager::onDnsCollectEndorseUnverified,
                                                               this,
                                                               _1,
                                                               identity)));
  m_wrapper->sendInterest(interestPtr, closure);
}

void
ContactManager::fetchKey(const ndn::Name& certName)
{
  Name interestName = certName;
  
  Ptr<Interest> interestPtr = Ptr<Interest>(new Interest(interestName));
  interestPtr->setChildSelector(Interest::CHILD_RIGHT);
  interestPtr->setInterestLifetime(1);
  Ptr<Closure> closure = Ptr<Closure> (new Closure(boost::bind(&ContactManager::onKeyVerified, 
                                                               this,
                                                               _1,
                                                               certName),
						   boost::bind(&ContactManager::onKeyTimeout,
                                                               this,
                                                               _1, 
                                                               _2,
                                                               certName,
                                                               0),
						   boost::bind(&ContactManager::onKeyUnverified,
                                                               this,
                                                               _1,
                                                               certName)));
  m_wrapper->sendInterest(interestPtr, closure);
}

void
ContactManager::fetchIdCertificate(const ndn::Name& certName)
{
  Name interestName = certName;
  
  Ptr<Interest> interestPtr = Ptr<Interest>(new Interest(interestName));
  interestPtr->setChildSelector(Interest::CHILD_RIGHT);
  interestPtr->setInterestLifetime(1);
  Ptr<Closure> closure = Ptr<Closure> (new Closure(boost::bind(&ContactManager::onIdCertificateVerified, 
                                                               this,
                                                               _1,
                                                               certName),
						   boost::bind(&ContactManager::onIdCertificateTimeout,
                                                               this,
                                                               _1, 
                                                               _2,
                                                               certName,
                                                               0),
						   boost::bind(&ContactManager::onIdCertificateUnverified,
                                                               this,
                                                               _1,
                                                               certName)));
  m_wrapper->sendInterest(interestPtr, closure);
}

void
ContactManager::onDnsCollectEndorseVerified(Ptr<Data> data, const Name& identity)
{ emit collectEndorseFetched (*data); }

void
ContactManager::onDnsCollectEndorseTimeout(Ptr<Closure> closure, Ptr<Interest> interest, const Name& identity, int retry)
{ emit collectEndorseFetchFailed (identity); }

void
ContactManager::onDnsCollectEndorseUnverified(Ptr<Data> data, const Name& identity)
{ emit collectEndorseFetchFailed (identity); }

void
ContactManager::onKeyVerified(Ptr<Data> data, const Name& identity)
{
  IdentityCertificate identityCertificate(*data);

  Ptr<ProfileData> profileData = Ptr<ProfileData>(new ProfileData(Profile(identityCertificate)));
  
  Ptr<IdentityManager> identityManager = m_keychain->getIdentityManager();
  Name certificateName = identityManager->getDefaultCertificateName ();
  identityManager->signByCertificate(*profileData, certificateName);

  Ptr<EndorseCertificate> endorseCertificate = NULL;
  try{
    endorseCertificate = Ptr<EndorseCertificate>(new EndorseCertificate(identityCertificate, profileData));
  }catch(exception& e){
    _LOG_ERROR("Exception: " << e.what());
    return;
  }

  identityManager->signByCertificate(*endorseCertificate, certificateName);

  emit contactKeyFetched (*endorseCertificate); 
}

void
ContactManager::onKeyUnverified(Ptr<Data> data, const Name& identity)
{ 
  _LOG_DEBUG("Key cannot be verified!");
  emit contactKeyFetchFailed (identity); 
}

void
ContactManager::onKeyTimeout(Ptr<Closure> closure, Ptr<Interest> interest, const Name& identity, int retry)
{ 
  _LOG_DEBUG("Key timeout!");
  emit contactKeyFetchFailed(identity); 
}

void
ContactManager::onIdCertificateVerified(Ptr<Data> data, const Name& identity)
{
  IdentityCertificate identityCertificate(*data);
  emit contactCertificateFetched(identityCertificate);
}

void
ContactManager::onIdCertificateUnverified(Ptr<Data> data, const Name& identity)
{ emit contactCertificateFetchFailed (identity); }

void
ContactManager::onIdCertificateTimeout(Ptr<Closure> closure, Ptr<Interest> interest, const Name& identity, int retry)
{ emit contactCertificateFetchFailed (identity); }

void
ContactManager::updateProfileData(const Name& identity)
{
  // Get current profile;
  Ptr<Profile> newProfile = m_contactStorage->getSelfProfile(identity);
  if(NULL == newProfile)
    return;
  Ptr<Blob> newProfileBlob = newProfile->toDerBlob();

  // Check if profile exists
  Ptr<Blob> profileDataBlob = m_contactStorage->getSelfEndorseCertificate(identity);
  if(NULL != profileDataBlob)
    {
      
      Ptr<EndorseCertificate> oldEndorseCertificate = NULL;
      try{
        Ptr<Data> plainData = Data::decodeFromWire(profileDataBlob);
        oldEndorseCertificate = Ptr<EndorseCertificate>(new EndorseCertificate(*plainData));
      }catch(exception& e){
        _LOG_ERROR("Exception: " << e.what());
        return;
      }

      const Blob& oldProfileBlob = oldEndorseCertificate->getProfileData()->content();

      if(oldProfileBlob == *newProfileBlob)
        return;

      Ptr<EndorseCertificate> newEndorseCertificate = getSignedSelfEndorseCertificate(identity, *newProfile);

      if(NULL == newEndorseCertificate)
        return;

      m_contactStorage->updateSelfEndorseCertificate(newEndorseCertificate, identity);

      publishSelfEndorseCertificateInDNS(newEndorseCertificate);
    }
  else
    {
      Ptr<EndorseCertificate> newEndorseCertificate = getSignedSelfEndorseCertificate(identity, *newProfile);

      if(NULL == newEndorseCertificate)
        return;

      m_contactStorage->addSelfEndorseCertificate(newEndorseCertificate, identity);

      publishSelfEndorseCertificateInDNS(newEndorseCertificate);
    }
}

void
ContactManager::updateEndorseCertificate(const ndn::Name& identity, const ndn::Name& signerIdentity)
{
  Ptr<Blob> oldEndorseCertificateBlob = m_contactStorage->getEndorseCertificate(identity);
  Ptr<EndorseCertificate> newEndorseCertificate = generateEndorseCertificate(identity, signerIdentity);
  if(NULL != oldEndorseCertificateBlob)
    {
      Ptr<EndorseCertificate> oldEndorseCertificate = NULL;
      try{
        Ptr<Data> plainData = Data::decodeFromWire(oldEndorseCertificateBlob);
        oldEndorseCertificate = Ptr<EndorseCertificate>(new EndorseCertificate(*plainData));
      }catch(exception& e){
        _LOG_ERROR("Exception: " << e.what());
        return;
      }
      const Blob& oldEndorseContent = oldEndorseCertificate->content();
      const Blob& newEndorseContent = newEndorseCertificate->content();
      if(oldEndorseContent == newEndorseContent)
        return;
    }
  else
    {
      if(NULL == newEndorseCertificate)
        return;
    }
  m_contactStorage->addEndorseCertificate(newEndorseCertificate, identity);
  publishEndorseCertificateInDNS(newEndorseCertificate, signerIdentity);
}

Ptr<EndorseCertificate> 
ContactManager::generateEndorseCertificate(const Name& identity, const Name& signerIdentity)
{
  Ptr<ContactItem> contact = getContact(identity);
  if(contact == NULL)
    return NULL;

  Ptr<IdentityManager> identityManager = m_keychain->getIdentityManager();
  Name signerKeyName = identityManager->getDefaultKeyNameForIdentity(signerIdentity);
  Name signerCertName = identityManager->getDefaultCertificateNameByIdentity(signerIdentity);

  vector<string> endorseList = m_contactStorage->getEndorseList(identity);

  Ptr<EndorseCertificate> cert = NULL;
  try{
    cert = Ptr<EndorseCertificate>(new EndorseCertificate(contact->getSelfEndorseCertificate(), signerKeyName, endorseList)); 
  }catch(exception& e){
    _LOG_ERROR("Exception: " << e.what());
    return NULL;
  } 
  identityManager->signByCertificate(*cert, signerCertName);

  return cert;
}

vector<Ptr<ContactItem> >
ContactManager::getContactItemList()
{ return m_contactStorage->getAllContacts(); }

Ptr<ContactItem>
ContactManager::getContact(const ndn::Name& contactNamespace)
{ return m_contactStorage->getContact(contactNamespace); }

Ptr<EndorseCertificate>
ContactManager::getSignedSelfEndorseCertificate(const Name& identity,
                                                const Profile& profile)
{
  Ptr<IdentityManager> identityManager = m_keychain->getIdentityManager();
  Name certificateName = identityManager->getDefaultCertificateNameByIdentity(identity);
  if(0 == certificateName.size())
    return NULL;

  Ptr<ProfileData> profileData = Ptr<ProfileData>(new ProfileData(profile));
  identityManager->signByCertificate(*profileData, certificateName);

  Ptr<security::IdentityCertificate> signingCert = identityManager->getCertificate(certificateName);
  if(NULL == signingCert)
    return NULL;

  Name signingKeyName = security::IdentityCertificate::certificateNameToPublicKeyName(signingCert->getName(), true);

  Ptr<security::IdentityCertificate> kskCert;
  if(signingKeyName.get(-1).toUri().substr(0,4) == string("dsk-"))
    {
      Ptr<const signature::Sha256WithRsa> dskCertSig = DynamicCast<const signature::Sha256WithRsa>(signingCert->getSignature());
      // HACK! KSK certificate should be retrieved from network.
      Name keyName = security::IdentityCertificate::certificateNameToPublicKeyName(dskCertSig->getKeyLocator().getKeyName());

      Name kskCertName = identityManager->getPublicStorage()->getDefaultCertificateNameForKey(keyName);

      kskCert = identityManager->getCertificate(kskCertName);

    }
  else
    {
      kskCert = signingCert;
    }

  if(NULL == kskCert)
    return NULL;

  vector<string> endorseList;
  Profile::const_iterator it = profile.begin();
  for(; it != profile.end(); it++)
    endorseList.push_back(it->first);
  
  Ptr<EndorseCertificate> selfEndorseCertificate = NULL;
  try{
    selfEndorseCertificate = Ptr<EndorseCertificate>(new EndorseCertificate(*kskCert,
                                                                            profileData,
                                                                            endorseList));
  }catch(exception& e){
    _LOG_ERROR("Exception: " << e.what());
    return NULL;
  } 

  identityManager->signByCertificate(*selfEndorseCertificate, kskCert->getName());

  return selfEndorseCertificate;
}


void
ContactManager::onDnsSelfEndorseCertificateVerified(Ptr<Data> data, const Name& identity)
{
  Ptr<Blob> dataContentBlob = Ptr<Blob>(new Blob(data->content().buf(), data->content().size()));

  Ptr<Data> plainData = NULL;
  Ptr<EndorseCertificate> selfEndorseCertificate = NULL;
  try{
    plainData = Data::decodeFromWire(dataContentBlob);
    selfEndorseCertificate = Ptr<EndorseCertificate>(new EndorseCertificate(*plainData));
  }catch(exception& e){
    _LOG_ERROR("Exception: " << e.what());
    return;
  }

  const security::Publickey& ksk = selfEndorseCertificate->getPublicKeyInfo();

  if(security::PolicyManager::verifySignature(*plainData, ksk))
    emit contactFetched (*selfEndorseCertificate); 
  else
    emit contactFetchFailed (identity);
}

void
ContactManager::onDnsSelfEndorseCertificateUnverified(Ptr<Data> data, const Name& identity)
{ emit contactFetchFailed (identity); }

void
ContactManager::onDnsSelfEndorseCertificateTimeout(Ptr<Closure> closure, Ptr<Interest> interest, const Name& identity, int retry)
{ emit contactFetchFailed(identity); }

void
ContactManager::publishSelfEndorseCertificateInDNS(Ptr<EndorseCertificate> selfEndorseCertificate)
{
  Ptr<Data> data = Ptr<Data>::Create();

  Name keyName = selfEndorseCertificate->getPublicKeyName();
  Name identity = keyName.getSubName(0, keyName.size()-1);


  Name dnsName = identity;
  dnsName.append("DNS").append("PROFILE").appendVersion();
  
  data->setName(dnsName);
  Ptr<Blob> blob = selfEndorseCertificate->encodeToWire();

  Content content(blob->buf(), blob->size());
  data->setContent(content);

  m_keychain->signByIdentity(*data, identity);

  m_dnsStorage->updateDnsSelfProfileData(*data, identity);
  
  Ptr<Blob> dnsBlob = data->encodeToWire();

  m_wrapper->putToNdnd(*dnsBlob);
}

void
ContactManager::publishEndorseCertificateInDNS(Ptr<EndorseCertificate> endorseCertificate, const Name& signerIdentity)
{
  Ptr<Data> data = Ptr<Data>::Create();

  Name keyName = endorseCertificate->getPublicKeyName();
  Name endorsee = keyName.getSubName(0, keyName.size()-1);


  Name dnsName = signerIdentity;
  dnsName.append("DNS").append(endorsee).append("ENDORSEE").appendVersion();
  
  data->setName(dnsName);
  Ptr<Blob> blob = endorseCertificate->encodeToWire();

  Content content(blob->buf(), blob->size());
  data->setContent(content);

  Name signCertName = m_keychain->getIdentityManager()->getDefaultCertificateNameByIdentity(signerIdentity);
  m_keychain->getIdentityManager()->signByCertificate(*data, signCertName);

  m_dnsStorage->updateDnsEndorseOthers(*data, signerIdentity, endorsee);
  
  Ptr<Blob> dnsBlob = data->encodeToWire();

  m_wrapper->putToNdnd(*dnsBlob);
}

void
ContactManager::publishEndorsedDataInDns(const Name& identity)
{
  Ptr<Data> data = Ptr<Data>::Create();

  Name dnsName = identity;
  dnsName.append("DNS").append("ENDORSED").appendVersion();
  data->setName(dnsName);
  
  Ptr<vector<Blob> > collectEndorseList = m_contactStorage->getCollectEndorseList(identity);

  Ptr<der::DerSequence> root = Ptr<der::DerSequence>::Create();

  vector<Blob>::const_iterator it = collectEndorseList->begin();
  for(; it != collectEndorseList->end(); it++)
    {
      Ptr<der::DerOctetString> entry = Ptr<der::DerOctetString>(new der::DerOctetString(*it));
      root->addChild(entry);
    }
  
  blob_stream blobStream;
  OutputIterator & start = reinterpret_cast<OutputIterator &> (blobStream);
  root->encode(start);

  Content content(blobStream.buf()->buf(), blobStream.buf()->size());
  data->setContent(content);
  
  Name signCertName = m_keychain->getIdentityManager()->getDefaultCertificateNameByIdentity(identity);
  m_keychain->getIdentityManager()->signByCertificate(*data, signCertName);

  m_dnsStorage->updateDnsOthersEndorse(*data, identity);
  
  Ptr<Blob> dnsBlob = data->encodeToWire();

  m_wrapper->putToNdnd(*dnsBlob);
}

void
ContactManager::addContact(const IdentityCertificate& identityCertificate, const Profile& profile)
{
  Ptr<ProfileData> profileData = Ptr<ProfileData>(new ProfileData(profile));
  
  Ptr<IdentityManager> identityManager = m_keychain->getIdentityManager();
  Name certificateName = identityManager->getDefaultCertificateNameByIdentity (m_defaultIdentity);
  identityManager->signByCertificate(*profileData, certificateName);

  Ptr<EndorseCertificate> endorseCertificate = NULL;
  try{
    endorseCertificate = Ptr<EndorseCertificate>(new EndorseCertificate(identityCertificate, profileData));
  }catch(exception& e){
    _LOG_ERROR("Exception: " << e.what());
    return;
  }

  identityManager->signByCertificate(*endorseCertificate, certificateName);

  ContactItem contactItem(*endorseCertificate);

  try{
    m_contactStorage->addContact(contactItem);
    emit contactAdded(contactItem.getNameSpace());
  }catch(exception& e){
    emit warning(e.what());
    _LOG_ERROR("Exception: " << e.what());
    return;
  }
}

void
ContactManager::removeContact(const ndn::Name& contactNameSpace)
{
  Ptr<ContactItem> contact = getContact(contactNameSpace);
  if(contact == NULL)
    return;
  m_contactStorage->removeContact(contactNameSpace);
  emit contactRemoved(contact->getPublicKeyName());
}


#if WAF
#include "contact-manager.moc"
#include "contact-manager.cpp.moc"
#endif
