/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "sec-policy-chrono-chat-invitation.h"
#include <ndn-cpp-dev/security/verifier.hpp>
#include <ndn-cpp-dev/security/signature-sha256-with-rsa.hpp>

#include "logging.h"

using namespace std;
using namespace ndn;
using namespace ndn::ptr_lib;

INIT_LOGGER("SecPolicyChronoChatInvitation");

SecPolicyChronoChatInvitation::SecPolicyChronoChatInvitation(const string& chatroomName,
                                                 const Name& signingIdentity,
                                                 int stepLimit)
  : m_chatroomName(chatroomName)
  , m_signingIdentity(signingIdentity)
  , m_stepLimit(stepLimit)
{
  m_invitationPolicyRule = make_shared<SecRuleRelative>("^<ndn><broadcast><chronos><invitation>([^<chatroom>]*)<chatroom>", 
                                                           "^([^<KEY>]*)<KEY>(<>*)[<dsk-.*><ksk-.*>]<ID-CERT>$", 
                                                           "==", "\\1", "\\1\\2", true);

  m_kskRegex = make_shared<Regex>("^([^<KEY>]*)<KEY>(<>*<ksk-.*>)<ID-CERT><>$", "\\1\\2");

  m_dskRule = make_shared<SecRuleRelative>("^([^<KEY>]*)<KEY><dsk-.*><ID-CERT><>$", 
                                              "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$", 
                                              "==", "\\1", "\\1\\2", true);

  m_keyNameRegex = make_shared<Regex>("^([^<KEY>]*)<KEY>(<>*<ksk-.*>)<ID-CERT>$", "\\1\\2");
} 

SecPolicyChronoChatInvitation::~SecPolicyChronoChatInvitation()
{}

bool 
SecPolicyChronoChatInvitation::skipVerifyAndTrust (const Data& data)
{ return false; }

bool
SecPolicyChronoChatInvitation::requireVerify (const Data& data)
{ return true; }

shared_ptr<ValidationRequest>
SecPolicyChronoChatInvitation::checkVerificationPolicy(const shared_ptr<Data>& data, 
                                                 int stepCount, 
                                                 const OnVerified& onVerified,
                                                 const OnVerifyFailed& onVerifyFailed)
{
  if(m_stepLimit == stepCount)
    {
      _LOG_ERROR("Reach the maximum steps of verification!");
      onVerifyFailed(data);
      return shared_ptr<ValidationRequest>();
    }

  try{
    SignatureSha256WithRsa sig(data->getSignature());    

    const Name & keyLocatorName = sig.getKeyLocator().getName();

    if(m_invitationPolicyRule->satisfy(*data))
      {
        // Name keyName = IdentityCertificate::certificateNameToPublicKeyName(keyLocatorName);
        // map<Name, PublicKey>::iterator it = m_trustAnchors.find(keyName);
        // if(m_trustAnchors.end() != it)
        //   {
        //     if(Sha256WithRsaHandler::verifySignature(*data, it->second))
        //       onVerified(data);
        //     else
        //       onVerifyFailed(data);

        //     return shared_ptr<ValidationRequest>();
        //   }

        shared_ptr<const Certificate> trustedCert = m_certificateCache.getCertificate(keyLocatorName);
      
        if(static_cast<bool>(trustedCert)){
          if(Verifier::verifySignature(*data, sig, trustedCert->getPublicKeyInfo()))
            onVerified(data);
          else
            onVerifyFailed(data);

          return shared_ptr<ValidationRequest>();
        }

        OnVerified recursiveVerifiedCallback = boost::bind(&SecPolicyChronoChatInvitation::onDskCertificateVerified, 
                                                           this, 
                                                           _1, 
                                                           data, 
                                                           onVerified, 
                                                           onVerifyFailed);
      
        OnVerifyFailed recursiveUnverifiedCallback = boost::bind(&SecPolicyChronoChatInvitation::onDskCertificateVerifyFailed, 
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
            if(it->second == identityCertificate.getPublicKeyInfo())
              {
                onVerified(data);
              }
            else
              onVerifyFailed(data);
          }
        else
          onVerifyFailed(data);

        return shared_ptr<ValidationRequest>();
      }

    if(m_dskRule->satisfy(*data))
      {
        m_keyNameRegex->match(keyLocatorName);
        Name keyName = m_keyNameRegex->expand();

        if(m_trustAnchors.end() != m_trustAnchors.find(keyName))
          if(Verifier::verifySignature(*data, sig, m_trustAnchors[keyName]))
            onVerified(data);
          else
            onVerifyFailed(data);
        else
          onVerifyFailed(data);

        return shared_ptr<ValidationRequest>();	
      }
  }catch(SignatureSha256WithRsa::Error &e){
    _LOG_DEBUG("checkVerificationPolicy " << e.what());
    onVerifyFailed(data);
    return shared_ptr<ValidationRequest>();
  }catch(KeyLocator::Error &e){
    _LOG_DEBUG("checkVerificationPolicy " << e.what());
    onVerifyFailed(data);
    return shared_ptr<ValidationRequest>();
  }

  onVerifyFailed(data);
  return shared_ptr<ValidationRequest>();
}

bool 
SecPolicyChronoChatInvitation::checkSigningPolicy(const Name& dataName, 
                                            const Name& certificateName)
{
  return true;
}
    
Name 
SecPolicyChronoChatInvitation::inferSigningIdentity(const Name& dataName)
{
  return m_signingIdentity;
}

void
SecPolicyChronoChatInvitation::addTrustAnchor(const EndorseCertificate& selfEndorseCertificate)
{ m_trustAnchors.insert(pair <Name, PublicKey > (selfEndorseCertificate.getPublicKeyName(), selfEndorseCertificate.getPublicKeyInfo())); }


// void
// SecPolicyChronoChatInvitation::addChatDataRule(const Name& prefix, 
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
SecPolicyChronoChatInvitation::onDskCertificateVerified(const shared_ptr<Data>& certData, 
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

      if(Verifier::verifySignature(*originalData, originalData->getSignature(), certificate->getPublicKeyInfo()))
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
SecPolicyChronoChatInvitation::onDskCertificateVerifyFailed(const shared_ptr<Data>& certData, 
                                                      shared_ptr<Data> originalData,
                                                      const OnVerifyFailed& onVerifyFailed)
{ onVerifyFailed(originalData); }

shared_ptr<IdentityCertificate> 
SecPolicyChronoChatInvitation::getValidatedDskCertificate(const ndn::Name& certName)
{
  map<Name, shared_ptr<IdentityCertificate> >::iterator it = m_dskCertificates.find(certName);
  if(m_dskCertificates.end() != it)
    return it->second;
  else
    return shared_ptr<IdentityCertificate>();
}
