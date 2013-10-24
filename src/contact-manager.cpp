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

  policyManager->addVerificationPolicyRule(Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<DNS>]*)<DNS><PROFILE>",
                                                                                          "^([^<KEY>]*)<KEY>(<>*)<><ID-CERT>",
                                                                                          "==", "\\1", "\\1\\2", true)));
  policyManager->addVerificationPolicyRule(Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<PROFILE-CERT>]*)<PROFILE-CERT>",
											  "^([^<KEY>]*)<KEY>(<>*<KSK-.*>)<ID-CERT>", 
											  "==", "\\1", "\\1\\2", true)));
  policyManager->addVerificationPolicyRule(Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<KEY>]*)<KEY>(<>*)<KSK-.*><ID-CERT>",
											  "^([^<KEY>]*)<KEY><DSK-.*><ID-CERT>",
											  ">", "\\1\\2", "\\1", true)));
  policyManager->addVerificationPolicyRule(Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<KEY>]*)<KEY><DSK-.*><ID-CERT>",
											  "^([^<KEY>]*)<KEY>(<>*)<KSK-.*><ID-CERT>",
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

vector<Ptr<ContactItem> >
ContactManager::getContactItemList()
{
  vector<Ptr<ContactItem> > result;
  
  vector<Ptr<ContactItem> > ncList = m_contactStorage->getAllNormalContacts();
  vector<Ptr<TrustedContact> > tcList = m_contactStorage->getAllTrustedContacts();

  result.insert(result.end(), tcList.begin(), tcList.end());
  result.insert(result.end(), ncList.begin(), ncList.end());

  return result;
}

Ptr<ContactItem>
ContactManager::getContact(const ndn::Name& contactNamespace)
{
  Ptr<ContactItem> contactItem = m_contactStorage->getNormalContact(contactNamespace);
  if(NULL != contactItem)
    return contactItem;
  
  contactItem = m_contactStorage->getTrustedContact(contactNamespace);
  if(NULL != contactItem)
    return contactItem;
  
  return NULL;
}

Ptr<EndorseCertificate>
ContactManager::getSignedSelfEndorseCertificate(const Name& identity,
                                                const Profile& profile)
{
  Ptr<IdentityManager> identityManager = m_keychain->getIdentityManager();
  Name certificateName = identityManager->getDefaultCertificateNameByIdentity(identity);
  if(0 == certificateName.size())
    return NULL;

  Ptr<ProfileData> profileData = Ptr<ProfileData>(new ProfileData(identity, profile));
  identityManager->signByCertificate(*profileData, certificateName);

  Ptr<security::IdentityCertificate> dskCert = identityManager->getCertificate(certificateName);
  Ptr<const signature::Sha256WithRsa> dskCertSig = DynamicCast<const signature::Sha256WithRsa>(dskCert->getSignature());
  // HACK! KSK certificate should be retrieved from network.
  Ptr<security::IdentityCertificate> kskCert = identityManager->getCertificate(dskCertSig->getKeyLocator().getKeyName());

  vector<string> endorseList;
  Profile::const_iterator it = profile.begin();
  for(; it != profile.end(); it++)
    endorseList.push_back(it->first);
  
  Ptr<EndorseCertificate> selfEndorseCertificate = Ptr<EndorseCertificate>(new EndorseCertificate(*kskCert,
                                                                                                  kskCert->getNotBefore(),
                                                                                                  kskCert->getNotAfter(),
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
{
  if(retry > 0)
    {
      Ptr<Closure> newClosure = Ptr<Closure>(new Closure(closure->m_dataCallback,
                                                         boost::bind(&ContactManager::onDnsSelfEndorseCertificateTimeout, 
                                                                     this, 
                                                                     _1, 
                                                                     _2, 
                                                                     identity,
                                                                     retry - 1),
                                                         closure->m_unverifiedCallback,
                                                         closure->m_stepCount)
                                             );
      m_wrapper->sendInterest(interest, newClosure);
    }
  else
    emit contactFetchFailed(identity);
}

void
ContactManager::publishSelfEndorseCertificateInDNS(Ptr<EndorseCertificate> selfEndorseCertificate)
{
  Ptr<Data> data = Ptr<Data>::Create();

  Name keyName = selfEndorseCertificate->getPublicKeyName();
  Name identity = keyName.getSubName(0, keyName.size()-1);

  TimeInterval ti = time::NowUnixTimestamp();
  ostringstream oss;
  oss << ti.total_seconds();

  Name dnsName = identity;
  dnsName.append("DNS").append("PROFILE").append(oss.str());
  
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


#if WAF
#include "contact-manager.moc"
#include "contact-manager.cpp.moc"
#endif
