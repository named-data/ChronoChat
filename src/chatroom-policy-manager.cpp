/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "chatroom-policy-manager.h"

#include <ndn.cxx/security/cache/ttl-certificate-cache.h>

#include "logging.h"

using namespace std;
using namespace ndn;
using namespace ndn::security;

INIT_LOGGER("ChatroomPolicyManager");

ChatroomPolicyManager::ChatroomPolicyManager(int stepLimit,
					     Ptr<CertificateCache> certificateCache)
  : m_stepLimit(stepLimit)
  , m_certificateCache(certificateCache)
{
  if(m_certificateCache == NULL)
    m_certificateCache = Ptr<TTLCertificateCache>(new TTLCertificateCache());

  m_invitationPolicyRule = Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^<ndn><broadcast><chronos><invitation>([^<chatroom>]*)<chatroom>", 
									  "^([^<KEY>]*)<KEY><DSK-.*><ID-CERT><>$", 
									  "==", "\\1", "\\1", true));

  m_dskRule = Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<KEY>]*)<KEY><DSK-.*><ID-CERT><>$", 
							     "^([^<KEY>]*)<KEY>(<>*)<KSK-.*><ID-CERT><>$", 
							     "==", "\\1", "\\1\\2", true));

  m_keyNameRegex = Ptr<Regex>(new Regex("^([^<KEY>]*)<KEY>(<>*<KSK-.*>)<ID-CERT><>$", "\\1\\2"));
} 

ChatroomPolicyManager::~ChatroomPolicyManager()
{}

bool 
ChatroomPolicyManager::skipVerifyAndTrust (const Data& data)
{ return false; }

bool
ChatroomPolicyManager::requireVerify (const Data& data)
{ return true; }

Ptr<ValidationRequest>
ChatroomPolicyManager::checkVerificationPolicy(Ptr<Data> data, 
					       const int& stepCount, 
					       const DataCallback& verifiedCallback,
					       const UnverifiedCallback& unverifiedCallback)
{
  if(m_stepLimit == stepCount)
    {
      _LOG_DEBUG("reach the maximum steps of verification");
      unverifiedCallback(data);
      return NULL;
    }

  Ptr<const signature::Sha256WithRsa> sha256sig = boost::dynamic_pointer_cast<const signature::Sha256WithRsa> (data->getSignature());    

  if(KeyLocator::KEYNAME != sha256sig->getKeyLocator().getType())
    {
      unverifiedCallback(data);
      return NULL;
    }

  const Name & keyLocatorName = sha256sig->getKeyLocator().getKeyName();

  if(m_invitationPolicyRule->satisfy(*data))
    {
      Ptr<const IdentityCertificate> trustedCert = m_certificateCache->getCertificate(keyLocatorName);
      
      if(NULL != trustedCert){
	if(verifySignature(*data, trustedCert->getPublicKeyInfo()))
	  verifiedCallback(data);
	else
	  unverifiedCallback(data);

	return NULL;
      }
      else{
	_LOG_DEBUG("KeyLocator has not been cached and validated!");

	DataCallback recursiveVerifiedCallback = boost::bind(&ChatroomPolicyManager::onCertificateVerified, 
							     this, 
							     _1, 
							     data, 
							     verifiedCallback, 
							     unverifiedCallback);

	UnverifiedCallback recursiveUnverifiedCallback = boost::bind(&ChatroomPolicyManager::onCertificateUnverified, 
								     this, 
								     _1, 
								     data, 
								     unverifiedCallback);


	Ptr<Interest> interest = Ptr<Interest>(new Interest(keyLocatorName));
	
	Ptr<ValidationRequest> nextStep = Ptr<ValidationRequest>(new ValidationRequest(interest, 
										       recursiveVerifiedCallback,
										       recursiveUnverifiedCallback,
										       0,
										       stepCount + 1)
								 );
	return nextStep;
      }
    }

  if(m_dskRule->satisfy(*data))
    {
      m_keyNameRegex->match(keyLocatorName);
      Name keyName = m_keyNameRegex->expand();

      if(m_trustAnchors.end() != m_trustAnchors.find(keyName))
        if(verifySignature(*data, m_trustAnchors[keyName]))
          verifiedCallback(data);
        else
          unverifiedCallback(data);
      else
        unverifiedCallback(data);

      return NULL;	
    }

  unverifiedCallback(data);
  return NULL;
}

bool 
ChatroomPolicyManager::checkSigningPolicy(const Name& dataName, 
					  const Name& certificateName)
{
  //TODO:
}
    
Name 
ChatroomPolicyManager::inferSigningIdentity(const Name& dataName)
{
  //TODO:
}

void
ChatroomPolicyManager::addTrustAnchor(const EndorseCertificate& selfEndorseCertificate)
{ m_trustAnchors.insert(pair <Name, Publickey > (selfEndorseCertificate.getPublicKeyName(), selfEndorseCertificate.getPublicKeyInfo())); }

void 
ChatroomPolicyManager::onCertificateVerified(Ptr<Data> certData, 
					     Ptr<Data> originalData,
					     const DataCallback& verifiedCallback, 
					     const UnverifiedCallback& unverifiedCallback)
{
  Ptr<IdentityCertificate> certificate = Ptr<IdentityCertificate>(new IdentityCertificate(*certData));
  m_certificateCache->insertCertificate(certificate);

  if(verifySignature(*originalData, certificate->getPublicKeyInfo()))
    verifiedCallback(originalData);
  else
    unverifiedCallback(originalData);
}

void
ChatroomPolicyManager::onCertificateUnverified(Ptr<Data> certData, 
					       Ptr<Data> originalData,
					       const UnverifiedCallback& unverifiedCallback)
{ unverifiedCallback(originalData); }

