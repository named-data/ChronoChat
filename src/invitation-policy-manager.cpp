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

#include <ndn.cxx/security/cache/ttl-certificate-cache.h>

#include "logging.h"

using namespace std;
using namespace ndn;
using namespace ndn::security;

INIT_LOGGER("InvitationPolicyManager");

InvitationPolicyManager::InvitationPolicyManager(const string& chatroomName,
                                                 const Name& signingIdentity,
                                                 int stepLimit,
                                                 Ptr<CertificateCache> certificateCache)
  : m_chatroomName(chatroomName)
  , m_signingIdentity(signingIdentity)
  , m_stepLimit(stepLimit)
  , m_certificateCache(certificateCache)
{
  if(m_certificateCache == NULL)
    m_certificateCache = Ptr<TTLCertificateCache>(new TTLCertificateCache());

  m_invitationPolicyRule = Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^<ndn><broadcast><chronos><invitation>([^<chatroom>]*)<chatroom>", 
									  "^([^<KEY>]*)<KEY>(<>*)[<dsk-.*><ksk-.*>]<ID-CERT>$", 
									  "==", "\\1", "\\1\\2", true));

  m_kskRegex = Ptr<Regex>(new Regex("^([^<KEY>]*)<KEY>(<>*<ksk-.*>)<ID-CERT><>$", "\\1\\2"));

  m_dskRule = Ptr<IdentityPolicyRule>(new IdentityPolicyRule("^([^<KEY>]*)<KEY><dsk-.*><ID-CERT><>$", 
							     "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$", 
							     "==", "\\1", "\\1\\2", true));

  m_keyNameRegex = Ptr<Regex>(new Regex("^([^<KEY>]*)<KEY>(<>*<ksk-.*>)<ID-CERT>$", "\\1\\2"));
} 

InvitationPolicyManager::~InvitationPolicyManager()
{}

bool 
InvitationPolicyManager::skipVerifyAndTrust (const Data& data)
{ return false; }

bool
InvitationPolicyManager::requireVerify (const Data& data)
{ return true; }

Ptr<ValidationRequest>
InvitationPolicyManager::checkVerificationPolicy(Ptr<Data> data, 
					       const int& stepCount, 
					       const DataCallback& verifiedCallback,
					       const UnverifiedCallback& unverifiedCallback)
{
  if(m_stepLimit == stepCount)
    {
      _LOG_ERROR("Reach the maximum steps of verification!");
      unverifiedCallback(data);
      return NULL;
    }

  Ptr<const signature::Sha256WithRsa> sha256sig = boost::dynamic_pointer_cast<const signature::Sha256WithRsa> (data->getSignature());    

  if(KeyLocator::KEYNAME != sha256sig->getKeyLocator().getType())
    {
      _LOG_ERROR("KeyLocator is not name!");
      unverifiedCallback(data);
      return NULL;
    }

  const Name & keyLocatorName = sha256sig->getKeyLocator().getKeyName();

  if(m_invitationPolicyRule->satisfy(*data))
    {
      // Name keyName = IdentityCertificate::certificateNameToPublicKeyName(keyLocatorName);
      // map<Name, Publickey>::iterator it = m_trustAnchors.find(keyName);
      // if(m_trustAnchors.end() != it)
      //   {
      //     if(verifySignature(*data, it->second))
      //       verifiedCallback(data);
      //     else
      //       unverifiedCallback(data);

      //     return NULL;
      //   }

      Ptr<const IdentityCertificate> trustedCert = m_certificateCache->getCertificate(keyLocatorName);
      
      if(NULL != trustedCert){
	if(verifySignature(*data, trustedCert->getPublicKeyInfo()))
	  verifiedCallback(data);
	else
	  unverifiedCallback(data);

	return NULL;
      }

      DataCallback recursiveVerifiedCallback = boost::bind(&InvitationPolicyManager::onDskCertificateVerified, 
                                                           this, 
                                                           _1, 
                                                           data, 
                                                           verifiedCallback, 
                                                           unverifiedCallback);
      
      UnverifiedCallback recursiveUnverifiedCallback = boost::bind(&InvitationPolicyManager::onDskCertificateUnverified, 
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

  if(m_kskRegex->match(data->getName()))
    {
      Name keyName = m_kskRegex->expand();
      map<Name, Publickey>::iterator it = m_trustAnchors.find(keyName);
      if(m_trustAnchors.end() != it)
        {
          Ptr<IdentityCertificate> identityCertificate = Ptr<IdentityCertificate>(new IdentityCertificate(*data));
          if(it->second.getKeyBlob() == identityCertificate->getPublicKeyInfo().getKeyBlob())
            {
              verifiedCallback(data);
            }
          else
            unverifiedCallback(data);
        }
      else
        unverifiedCallback(data);

      return NULL;
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
InvitationPolicyManager::checkSigningPolicy(const Name& dataName, 
					  const Name& certificateName)
{
  return true;
}
    
Name 
InvitationPolicyManager::inferSigningIdentity(const Name& dataName)
{
  return m_signingIdentity;
}

void
InvitationPolicyManager::addTrustAnchor(const EndorseCertificate& selfEndorseCertificate)
{ m_trustAnchors.insert(pair <Name, Publickey > (selfEndorseCertificate.getPublicKeyName(), selfEndorseCertificate.getPublicKeyInfo())); }


// void
// InvitationPolicyManager::addChatDataRule(const Name& prefix, 
//                                        const IdentityCertificate identityCertificate)
// {
//   Name dataPrefix = prefix;
//   dataPrefix.append("chronos").append(m_chatroomName);
//   Ptr<Regex> dataRegex = Regex::fromName(prefix);
//   Name certName = identityCertificate.getName();
//   Name signerName = certName.getPrefix(certName.size()-1);
//   Ptr<Regex> signerRegex = Regex::fromName(signerName, true);
  
//   ChatPolicyRule rule(dataRegex, signerRegex);
//   map<Name, ChatPolicyRule>::iterator it = m_chatDataRules.find(dataPrefix);
//   if(it != m_chatDataRules.end())
//     it->second = rule;
//   else
//     m_chatDataRules.insert(pair <Name, ChatPolicyRule > (dataPrefix, rule));
// }


void 
InvitationPolicyManager::onDskCertificateVerified(Ptr<Data> certData, 
                                                  Ptr<Data> originalData,
                                                  const DataCallback& verifiedCallback, 
                                                  const UnverifiedCallback& unverifiedCallback)
{
  Ptr<IdentityCertificate> certificate = Ptr<IdentityCertificate>(new IdentityCertificate(*certData));

  if(!certificate->isTooLate() && !certificate->isTooEarly())
    {
      Name certName = certificate->getName().getPrefix(certificate->getName().size()-1);
      map<Name, Ptr<IdentityCertificate> >::iterator it = m_dskCertificates.find(certName);
      if(it == m_dskCertificates.end())
        m_dskCertificates.insert(pair <Name, Ptr<IdentityCertificate> > (certName, certificate));

      if(verifySignature(*originalData, certificate->getPublicKeyInfo()))
        {
          verifiedCallback(originalData);
          return;
        }
    }
  else
    {
      unverifiedCallback(originalData);
      return;
    }
}

void
InvitationPolicyManager::onDskCertificateUnverified(Ptr<Data> certData, 
                                                  Ptr<Data> originalData,
                                                  const UnverifiedCallback& unverifiedCallback)
{ unverifiedCallback(originalData); }

Ptr<IdentityCertificate> 
InvitationPolicyManager::getValidatedDskCertificate(const ndn::Name& certName)
{
  map<Name, Ptr<IdentityCertificate> >::iterator it = m_dskCertificates.find(certName);
  if(m_dskCertificates.end() != it)
    return it->second;
  else
    return NULL;
}
