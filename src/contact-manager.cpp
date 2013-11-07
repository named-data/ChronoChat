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
#include <ndn.cxx/security/identity/basic-identity-storage.h>
#include <ndn.cxx/security/identity/osx-privatekey-storage.h>
#include <ndn.cxx/security/policy/simple-policy-manager.h>
#include <ndn.cxx/security/policy/identity-policy-rule.h>
#include <ndn.cxx/security/cache/ttl-certificate-cache.h>
#include <ndn.cxx/security/encryption/basic-encryption-manager.h>
#include <ndn.cxx/helpers/der/der.h>
#include <fstream>
#include "logging.h"
#endif

using namespace ndn;
using namespace ndn::security;

INIT_LOGGER("ContactManager");

ContactManager::ContactManager(Ptr<ContactStorage> contactStorage,
                               Ptr<DnsStorage> dnsStorage,
                               QObject* parent)
  : QObject(parent)
  , m_contactStorage(contactStorage)
  , m_dnsStorage(dnsStorage)
{
  setKeychain();

  m_wrapper = Ptr<Wrapper>(new Wrapper(m_keychain));
}

ContactManager::~ContactManager()
{
}

void
ContactManager::setKeychain()
{
  Ptr<OSXPrivatekeyStorage> privateStorage = Ptr<OSXPrivatekeyStorage>::Create();
  Ptr<IdentityManager> identityManager = Ptr<IdentityManager>(new IdentityManager(Ptr<BasicIdentityStorage>::Create(), privateStorage));
  Ptr<TTLCertificateCache> certificateCache = Ptr<TTLCertificateCache>(new TTLCertificateCache());
  Ptr<SimplePolicyManager> policyManager = Ptr<SimplePolicyManager>(new SimplePolicyManager(10, certificateCache));
  Ptr<EncryptionManager> encryptionManager = Ptr<EncryptionManager>(new BasicEncryptionManager(privateStorage, "/tmp/encryption.db"));
  Ptr<Keychain> keychain = Ptr<Keychain>(new Keychain(identityManager, policyManager, encryptionManager));

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

  policyManager->addSigningPolicyRule(Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<DNS>]*)<DNS><PROFILE>",
                                                                                     "^([^<KEY>]*)<KEY>(<>*)<><ID-CERT>",
                                                                                     "==", "\\1", "\\1\\2", true)));

  ifstream is ("trust-anchor.data", ios::binary);
  is.seekg (0, ios::end);
  ifstream::pos_type size = is.tellg();
  char * memblock = new char [size];    
  is.seekg (0, ios::beg);
  is.read (memblock, size);
  is.close();

  Ptr<Blob> readBlob = Ptr<Blob>(new Blob(memblock, size));
  Ptr<Data> readData = Data::decodeFromWire (readBlob);
  Ptr<IdentityCertificate> anchor = Ptr<IdentityCertificate>(new IdentityCertificate(*readData));   
  policyManager->addTrustAnchor(anchor);  
  
  delete memblock;

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

  EndorseCertificate endorseCertificate(identityCertificate, profileData);

  identityManager->signByCertificate(endorseCertificate, certificateName);

  emit contactKeyFetched (endorseCertificate); 
}

void
ContactManager::onKeyUnverified(Ptr<Data> data, const Name& identity)
{ emit contactKeyFetchFailed (identity); }

void
ContactManager::onKeyTimeout(Ptr<Closure> closure, Ptr<Interest> interest, const Name& identity, int retry)
{ emit contactKeyFetchFailed(identity); }

void
ContactManager::updateProfileData(const Name& identity)
{
  _LOG_DEBUG("updateProfileData: " << identity.toUri());
  // Get current profile;
  Ptr<Profile> newProfile = m_contactStorage->getSelfProfile(identity);
  if(NULL == newProfile)
    return;
  Ptr<Blob> newProfileBlob = newProfile->toDerBlob();

  // Check if profile exists
  Ptr<Blob> profileDataBlob = m_contactStorage->getSelfEndorseCertificate(identity);
  if(NULL != profileDataBlob)
    {
      Ptr<Data> plainData = Data::decodeFromWire(profileDataBlob);
      EndorseCertificate oldEndorseCertificate(*plainData);    
      // _LOG_DEBUG("Certificate converted!");
      const Blob& oldProfileBlob = oldEndorseCertificate.getProfileData()->content();

      if(oldProfileBlob == *newProfileBlob)
        return;

      Ptr<EndorseCertificate> newEndorseCertificate = getSignedSelfEndorseCertificate(identity, *newProfile);
      // _LOG_DEBUG("Signing DONE!");
      if(NULL == newEndorseCertificate)
        return;
      _LOG_DEBUG("About to update");
      m_contactStorage->updateSelfEndorseCertificate(newEndorseCertificate, identity);

      publishSelfEndorseCertificateInDNS(newEndorseCertificate);
    }
  else
    {
      Ptr<EndorseCertificate> newEndorseCertificate = getSignedSelfEndorseCertificate(identity, *newProfile);
      // _LOG_DEBUG("Signing DONE!");
      if(NULL == newEndorseCertificate)
        return;
      _LOG_DEBUG("About to Insert");
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
      Ptr<Data> plainData = Data::decodeFromWire(oldEndorseCertificateBlob);
      EndorseCertificate oldEndorseCertificate(*plainData);
      const Blob& oldEndorseContent = oldEndorseCertificate.content();
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

  Ptr<IdentityManager> identityManager = m_keychain->getIdentityManager();
  Name signerKeyName = identityManager->getDefaultKeyNameForIdentity(signerIdentity);
  Name signerCertName = identityManager->getDefaultCertificateNameByIdentity(signerIdentity);

  vector<string> endorseList = m_contactStorage->getEndorseList(identity);

  Ptr<EndorseCertificate> cert = Ptr<EndorseCertificate>(new EndorseCertificate(contact->getSelfEndorseCertificate(), signerKeyName, endorseList)); 
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
  Name signingKeyName = security::IdentityCertificate::certificateNameToPublicKeyName(signingCert->getName(), true);

  Ptr<security::IdentityCertificate> kskCert;
  if(signingKeyName.get(-1).toUri().substr(0,4) == string("dsk-"))
    {
      Ptr<const signature::Sha256WithRsa> dskCertSig = DynamicCast<const signature::Sha256WithRsa>(signingCert->getSignature());
      // HACK! KSK certificate should be retrieved from network.
      _LOG_DEBUG("keyLocator: " << dskCertSig->getKeyLocator().getKeyName());
      Name keyName = security::IdentityCertificate::certificateNameToPublicKeyName(dskCertSig->getKeyLocator().getKeyName());
      _LOG_DEBUG("keyName: " << keyName.toUri());
      Name kskCertName = identityManager->getPublicStorage()->getDefaultCertificateNameForKey(keyName);
      _LOG_DEBUG("ksk cert name: " << kskCertName);
      kskCert = identityManager->getCertificate(kskCertName);

    }
  else
    {
      kskCert = signingCert;
      _LOG_DEBUG("ksk cert name: " << kskCert->getName().toUri());
    }

  vector<string> endorseList;
  Profile::const_iterator it = profile.begin();
  for(; it != profile.end(); it++)
    endorseList.push_back(it->first);
  
  Ptr<EndorseCertificate> selfEndorseCertificate = Ptr<EndorseCertificate>(new EndorseCertificate(*kskCert,
                                                                                                  profileData,
                                                                                                  endorseList));
  identityManager->signByCertificate(*selfEndorseCertificate, kskCert->getName());

  return selfEndorseCertificate;
}


void
ContactManager::onDnsSelfEndorseCertificateVerified(Ptr<Data> data, const Name& identity)
{
  Ptr<Blob> dataContentBlob = Ptr<Blob>(new Blob(data->content().buf(), data->content().size()));

  Ptr<Data> plainData = Data::decodeFromWire(dataContentBlob);

  Ptr<EndorseCertificate> selfEndorseCertificate = Ptr<EndorseCertificate>(new EndorseCertificate(*plainData));

  const security::Publickey& ksk = selfEndorseCertificate->getPublicKeyInfo();

  if(security::PolicyManager::verifySignature(*plainData, ksk))
    {
      // Profile profile = selfEndorseCertificate->getProfileData()->getProfile();
      // Profile::const_iterator it = profile.getEntries().begin();
      // it++;
      // _LOG_DEBUG("Entry Size: " << it->first);

      emit contactFetched (*selfEndorseCertificate); 
    }
  else
    {
      emit contactFetchFailed (identity);
    }
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

  // string encoded;
  // CryptoPP::StringSource ss(reinterpret_cast<const unsigned char *>(blob->buf()), blob->size(), true,
  //       		    new CryptoPP::Base64Encoder(new CryptoPP::StringSink(encoded), false));

  // Content content(encoded.c_str(), encoded.size());
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


#if WAF
#include "contact-manager.moc"
#include "contact-manager.cpp.moc"
#endif
