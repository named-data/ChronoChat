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
#include "null-ptrs.h"
#include <ndn-cpp/sha256-with-rsa-signature.hpp>
#include <ndn-cpp/security/signature/sha256-with-rsa-handler.hpp>

#include "logging.h"

using namespace std;
using namespace ndn;
using namespace ndn::ptr_lib;

INIT_LOGGER("InvitationPolicyManager");

InvitationPolicyManager::InvitationPolicyManager(const string& chatroomName,
                                                 const Name& signingIdentity,
                                                 int stepLimit)
  : m_chatroomName(chatroomName)
  , m_signingIdentity(signingIdentity)
  , m_stepLimit(stepLimit)
{
  m_invitationPolicyRule = make_shared<IdentityPolicyRule>("^<ndn><broadcast><chronos><invitation>([^<chatroom>]*)<chatroom>", 
                                                           "^([^<KEY>]*)<KEY>(<>*)[<dsk-.*><ksk-.*>]<ID-CERT>$", 
                                                           "==", "\\1", "\\1\\2", true);

  m_kskRegex = make_shared<Regex>("^([^<KEY>]*)<KEY>(<>*<ksk-.*>)<ID-CERT><>$", "\\1\\2");

  m_dskRule = make_shared<IdentityPolicyRule>("^([^<KEY>]*)<KEY><dsk-.*><ID-CERT><>$", 
                                              "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$", 
                                              "==", "\\1", "\\1\\2", true);

  m_keyNameRegex = make_shared<Regex>("^([^<KEY>]*)<KEY>(<>*<ksk-.*>)<ID-CERT>$", "\\1\\2");
} 

InvitationPolicyManager::~InvitationPolicyManager()
{}

bool 
InvitationPolicyManager::skipVerifyAndTrust (const Data& data)
{ return false; }

bool
InvitationPolicyManager::requireVerify (const Data& data)
{ return true; }

shared_ptr<ValidationRequest>
InvitationPolicyManager::checkVerificationPolicy(const shared_ptr<Data>& data, 
                                                 int stepCount, 
                                                 const OnVerified& onVerified,
                                                 const OnVerifyFailed& onVerifyFailed)
{
  if(m_stepLimit == stepCount)
    {
      _LOG_ERROR("Reach the maximum steps of verification!");
      onVerifyFailed(data);
      return CHRONOCHAT_NULL_VALIDATIONREQUEST_PTR;
    }

  const Sha256WithRsaSignature* sha256sig = dynamic_cast<const Sha256WithRsaSignature*> (data->getSignature());    

  if(ndn_KeyLocatorType_KEYNAME != sha256sig->getKeyLocator().getType())
    {
      _LOG_ERROR("KeyLocator is not name!");
      onVerifyFailed(data);
      return CHRONOCHAT_NULL_VALIDATIONREQUEST_PTR;
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

      shared_ptr<const Certificate> trustedCert = m_certificateCache.getCertificate(keyLocatorName);
      
      if(!trustedCert){
	if(Sha256WithRsaHandler::verifySignature(*data, trustedCert->getPublicKeyInfo()))
	  onVerified(data);
	else
	  onVerifyFailed(data);

	return CHRONOCHAT_NULL_VALIDATIONREQUEST_PTR;
      }

      OnVerified recursiveVerifiedCallback = boost::bind(&InvitationPolicyManager::onDskCertificateVerified, 
                                                         this, 
                                                         _1, 
                                                         data, 
                                                         onVerified, 
                                                         onVerifyFailed);
      
      OnVerifyFailed recursiveUnverifiedCallback = boost::bind(&InvitationPolicyManager::onDskCertificateVerifyFailed, 
                                                               this, 
                                                               _1, 
                                                               data, 
                                                               onVerifyFailed);


      shared_ptr<Interest> interest = make_shared<Interest>(keyLocatorName);
      
      shared_ptr<ValidationRequest> nextStep = make_shared<ValidationRequest>(interest, 
                                                                              recursiveVerifiedCallback,
                                                                              recursiveUnverifiedCallback,
                                                                              0,
                                                                              stepCount + 1);
      return nextStep;
    }

  if(m_kskRegex->match(data->getName()))
    {
      Name keyName = m_kskRegex->expand();
      map<Name, PublicKey>::iterator it = m_trustAnchors.find(keyName);
      if(m_trustAnchors.end() != it)
        {
          IdentityCertificate identityCertificate(*data);
          if(isSameKey(it->second.getKeyDer(), identityCertificate.getPublicKeyInfo().getKeyDer()))
            {
              onVerified(data);
            }
          else
            onVerifyFailed(data);
        }
      else
        onVerifyFailed(data);

      return CHRONOCHAT_NULL_VALIDATIONREQUEST_PTR;
    }

  if(m_dskRule->satisfy(*data))
    {
      m_keyNameRegex->match(keyLocatorName);
      Name keyName = m_keyNameRegex->expand();

      if(m_trustAnchors.end() != m_trustAnchors.find(keyName))
        if(Sha256WithRsaHandler::verifySignature(*data, m_trustAnchors[keyName]))
          onVerified(data);
        else
          onVerifyFailed(data);
      else
        onVerifyFailed(data);

      return CHRONOCHAT_NULL_VALIDATIONREQUEST_PTR;	
    }

  onVerifyFailed(data);
  return CHRONOCHAT_NULL_VALIDATIONREQUEST_PTR;
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
{ m_trustAnchors.insert(pair <Name, PublicKey > (selfEndorseCertificate.getPublicKeyName(), selfEndorseCertificate.getPublicKeyInfo())); }


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
InvitationPolicyManager::onDskCertificateVerified(const shared_ptr<Data>& certData, 
                                                  shared_ptr<Data> originalData,
                                                  const OnVerified& onVerified, 
                                                  const OnVerifyFailed& onVerifyFailed)
{
  shared_ptr<IdentityCertificate> certificate = make_shared<IdentityCertificate>(*certData);

  if(!certificate->isTooLate() && !certificate->isTooEarly())
    {
      Name certName = certificate->getName().getPrefix(-1);
      map<Name, shared_ptr<IdentityCertificate> >::iterator it = m_dskCertificates.find(certName);
      if(it == m_dskCertificates.end())
        m_dskCertificates.insert(pair <Name, shared_ptr<IdentityCertificate> > (certName, certificate));

      if(Sha256WithRsaHandler::verifySignature(*originalData, certificate->getPublicKeyInfo()))
        {
          onVerified(originalData);
          return;
        }
    }
  else
    {
      onVerifyFailed(originalData);
      return;
    }
}

void
InvitationPolicyManager::onDskCertificateVerifyFailed(const shared_ptr<Data>& certData, 
                                                      shared_ptr<Data> originalData,
                                                      const OnVerifyFailed& onVerifyFailed)
{ onVerifyFailed(originalData); }

shared_ptr<IdentityCertificate> 
InvitationPolicyManager::getValidatedDskCertificate(const ndn::Name& certName)
{
  map<Name, shared_ptr<IdentityCertificate> >::iterator it = m_dskCertificates.find(certName);
  if(m_dskCertificates.end() != it)
    return it->second;
  else
    return CHRONOCHAT_NULL_IDENTITYCERTIFICATE_PTR;
}


bool
InvitationPolicyManager::isSameKey(const Blob& keyA, const Blob& keyB)
{
  size_t size = keyA.size();

  if(size != keyB.size())
    return false;

  const uint8_t* ap = keyA.buf();
  const uint8_t* bp = keyB.buf();
  
  for(int i = 0; i < size; i++)
    {
      if(ap[i] != bp[i])
        return false;
    }

  return true;
}
