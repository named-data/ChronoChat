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

#include <ndn.cxx/wrapper/wrapper.h>
#include <ndn.cxx/security/keychain.h>
#include <ndn.cxx/security/identity/basic-identity-storage.h>
#include <ndn.cxx/security/identity/osx-privatekey-storage.h>
#include <ndn.cxx/security/policy/simple-policy-manager.h>
#include <ndn.cxx/security/policy/identity-policy-rule.h>
#include <ndn.cxx/security/cache/ttl-certificate-cache.h>
#include <ndn.cxx/security/encryption/basic-encryption-manager.h>

#include <fstream>

using namespace ndn;
using namespace ndn::security;

ContactManager::ContactManager(Ptr<ContactStorage> contactStorage)
  : m_contactStorage(contactStorage)
{
  
  m_wrapper = Ptr<Wrapper>(new Wrapper(setKeychain()));
}

ContactManager::~ContactManager()
{
}

Ptr<Keychain>
ContactManager::setKeychain()
{
  Ptr<OSXPrivatekeyStorage> privateStorage = Ptr<OSXPrivatekeyStorage>::Create();
  Ptr<IdentityManager> identityManager = Ptr<IdentityManager>(new IdentityManager(Ptr<BasicIdentityStorage>::Create(), privateStorage));
  Ptr<TTLCertificateCache> certificateCache = Ptr<TTLCertificateCache>(new TTLCertificateCache());
  Ptr<SimplePolicyManager> policyManager = Ptr<SimplePolicyManager>(new SimplePolicyManager(10, certificateCache));
  Ptr<EncryptionManager> encryptionManager = Ptr<EncryptionManager>(new BasicEncryptionManager(privateStorage, "/tmp/encryption.db"));
  Ptr<Keychain> keychain = Ptr<Keychain>(new Keychain(identityManager, policyManager, encryptionManager));

  policyManager->addVerificationPolicyRule(Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<PROFILE-CERT>]*)<PROFILE-CERT>",
											  "^([^<KEY>]*)<KEY>(<>*<KSK-.*>)<ID-CERT>", 
											  "==", "\\1", "\\1\\2", false)));
  policyManager->addVerificationPolicyRule(Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<KEY>]*)<KEY>(<>*)<KSK-.*><ID-CERT>",
											  "^([^<KEY>]*)<KEY><DSK-.*><ID-CERT>",
											  ">", "\\1\\2", "\\1", false)));
  policyManager->addVerificationPolicyRule(Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<KEY>]*)<KEY><DSK-.*><ID-CERT>",
											  "^([^<KEY>]*)<KEY>(<>*)<KSK-.*><ID-CERT>",
											  "==", "\\1", "\\1\\2", false)));

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

  return keychain;
}
