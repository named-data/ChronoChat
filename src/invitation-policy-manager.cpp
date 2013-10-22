/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "invitation-policy-manager.h"
#include "logging.h"

using namespace std;
using namespace ndn;
using namespace ndn::security;

INIT_LOGGER("InvitationPolicyManager");

InvitationPolicyManager::InvitationPolicyManager(const int & stepLimit,
						 Ptr<CertificateCache> certificateCache,
						 Name signingIdentity)
  : m_stepLimit(stepLimit)
  , m_certificateCache(certificateCache)
  , m_localPrefixRegex(Ptr<Regex>(new Regex("^<local><ndn><prefix><><>$")))
{
  m_invitationDataRule = Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^<ndn><broadcast><chronos><invitation>([^<chatroom>]*)<chatroom>", 
									"^([^<KEY>]*)<KEY><DSK-.*><ID-CERT>$", 
									"==", "\\1", "\\1", true));
  
  m_dskRule = Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<KEY>]*)<KEY><DSK-.*><ID-CERT>$", 
							     "^([^<KEY>]*)<KEY>(<>*)<KSK-.*><ID-CERT>$", 
							     "==", "\\1", "\\1\\2", true));
  m_signingCertificateRegex = Ptr<Regex>(new Regex("^<ndn><broadcast><chronos><invitation>([^<chatroom>]*)<chatroom>", "\\1"));
}

bool 
InvitationPolicyManager::skipVerifyAndTrust (const Data & data)
{
  if(m_localPrefixRegex->match(data.getName()))
    return true;
  
  return false;
}

bool
InvitationPolicyManager::requireVerify (const Data & data)
{
  if(m_invitationDataRule->matchDataName(data))
    return true;

  if(m_dskRule->matchDataName(data))
    return true;

  return false;
}

Ptr<ValidationRequest>
InvitationPolicyManager::checkVerificationPolicy(Ptr<Data> data, 
						 const int & stepCount, 
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

  if(m_invitationDataRule->satisfy(*data))
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

	DataCallback recursiveVerifiedCallback = boost::bind(&InvitationPolicyManager::onCertificateVerified, 
							     this, 
							     _1, 
							     data, 
							     verifiedCallback, 
							     unverifiedCallback);

	UnverifiedCallback recursiveUnverifiedCallback = boost::bind(&InvitationPolicyManager::onCertificateUnverified, 
								     this, 
								     _1, 
								     data, 
								     unverifiedCallback);


	Ptr<Interest> interest = Ptr<Interest>(new Interest(sha256sig->getKeyLocator().getKeyName()));
	
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
      Ptr<IdentityCertificate> trustedCert;
      if(m_trustAnchors.end() != m_trustAnchors.find(keyLocatorName))
	trustedCert = m_trustAnchors[keyLocatorName];
      else
	{
	  unverifiedCallback(data);
	  return NULL;
	}

      if(verifySignature(*data, trustedCert->getPublicKeyInfo()))
	verifiedCallback(data);
      else
	unverifiedCallback(data);

      return NULL;	
    }
}

void 
InvitationPolicyManager::onCertificateVerified(Ptr<Data> certData, 
					       Ptr<Data> originalData,
					       const DataCallback& verifiedCallback, 
					       const UnverifiedCallback& unverifiedCallback)
{
  IdentityCertificate certificate(*certData);

  if(verifySignature(*originalData, certificate.getPublicKeyInfo()))
    verifiedCallback(originalData);
  else
    unverifiedCallback(originalData);

  return NULL;
}

void
InvitationPolicyManager::onCertificateUnverified(Ptr<Data> certData, 
						 Ptr<Data> originalData,
						 const UnverifiedCallback& unverifiedCallback)
{ unverifiedCallback(originalData); }

bool 
InvitationPolicyManager::checkSigningPolicy(const Name & dataName, const Name & certificateName)
{
  return m_invitationDataRule->satisfy(dataName, certificateName);
}

Name 
InvitationPolicyManager::inferSigningIdentity(const Name & dataName)
{
  if(m_signingCertificateRegex->match(data))
    return m_signingCertificateRegex->expand();
  else
    return Name();
}

void
InvitationPolicyManager::addTrustAnchor(Ptr<IdentityCertificate> ksk)
{
  Name name = ksk->getName();    
  m_cache.insert(pair <Name, Ptr<IdentityCertificate> > (name, ksk));
}
