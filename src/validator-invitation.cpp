/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "validator-invitation.h"

#include "logging.h"

using namespace std;
using namespace ndn;

INIT_LOGGER("ValidatorInvitation");

namespace chronos{

const shared_ptr<CertificateCache> ValidatorInvitation::DefaultCertificateCache = shared_ptr<CertificateCache>();

ValidatorInvitation::ValidatorInvitation(shared_ptr<Face> face,                                        
                                         const string& chatroomName,
                                         const Name& signingIdentity,
                                         shared_ptr<CertificateCache> certificateCache,
                                         int stepLimit)
  : Validator(face)
  , m_stepLimit(stepLimit)
  , m_certificateCache(certificateCache)
  , m_chatroomName(chatroomName)
  , m_signingIdentity(signingIdentity)
{
  m_invitationRule = make_shared<SecRuleRelative>("^<ndn><broadcast><chronos><invitation>([^<chatroom>]*)<chatroom>", 
                                                  "^([^<KEY>]*)<KEY>(<>*)[<dsk-.*><ksk-.*>]<ID-CERT>$", 
                                                  "==", "\\1", "\\1\\2", true);

  m_dskRule = make_shared<SecRuleRelative>("^([^<KEY>]*)<KEY><dsk-.*><ID-CERT><>$", 
                                           "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$", 
                                           "==", "\\1", "\\1\\2", true);
} 

void
ValidatorInvitation::checkPolicy (const shared_ptr<const Data>& data, 
                                  int stepCount, 
                                  const OnDataValidated& onValidated, 
                                  const OnDataValidationFailed& onValidationFailed,
                                  vector<shared_ptr<ValidationRequest> >& nextSteps)
{
  if(m_stepLimit == stepCount)
    {
      _LOG_DEBUG("reach the maximum steps of verification");
      onValidationFailed(data);
      return;
    }

   try{
    SignatureSha256WithRsa sig(data->getSignature());    
    const Name & keyLocatorName = sig.getKeyLocator().getName();
    const uint8_t* buf = data->wireEncode().wire();
    const size_t size = data->wireEncode().size();

    if(m_invitationRule->satisfy(data->getName(), keyLocatorName))
      processSignature(buf, size,
                       sig, keyLocatorName, 
                       bind(onValidated, data),
                       bind(onValidationFailed, data),
                       stepCount,
                       nextSteps);

    if(m_dskRule->satisfy(data->getName(), keyLocatorName))
      processFinalSignature(buf, size,
                            sig, keyLocatorName,
                            bind(onValidated, data),
                            bind(onValidationFailed, data));

  }catch(...){
    onValidationFailed(data);
    return;
  }
}

void
ValidatorInvitation::checkPolicy (const shared_ptr<const Interest>& interest, 
                                  int stepCount, 
                                  const OnInterestValidated& onValidated, 
                                  const OnInterestValidationFailed& onValidationFailed,
                                  vector<shared_ptr<ValidationRequest> >& nextSteps)
{
  try{    
    Name interestName  = interest->getName();

    Block signatureBlock = interestName.get(-1).blockFromValue();
    Block signatureInfo = interestName.get(-2).blockFromValue();
    Signature signature(signatureInfo, signatureBlock);
    
    SignatureSha256WithRsa sig(signature);
    const Name & keyLocatorName = sig.getKeyLocator().getName();

    Name signedName = interestName.getPrefix(-1);
    Buffer signedBlob = Buffer(signedName.wireEncode().value(), signedName.wireEncode().value_size());

    processSignature(signedBlob.buf(), signedBlob.size(),
                     sig, keyLocatorName, 
                     bind(onValidated, interest),
                     bind(onValidationFailed, interest),
                     stepCount,
                     nextSteps);

  }catch(...){
    onValidationFailed(interest);
    return;
  }

}

void
ValidatorInvitation::processSignature (const uint8_t* buf, 
                                       const size_t size,
                                       const SignatureSha256WithRsa& signature,
                                       const Name& keyLocatorName,
                                       const OnValidated& onValidated, 
                                       const OnValidationFailed& onValidationFailed,
                                       int stepCount,
                                       vector<shared_ptr<ValidationRequest> >& nextSteps)
{
  try{
    Name keyName = IdentityCertificate::certificateNameToPublicKeyName(keyLocatorName);
    
    if(m_trustAnchors.find(keyName) != m_trustAnchors.end())
      {
        if(Validator::verifySignature(buf, size, signature, m_trustAnchors[keyName]))
          onValidated();
        else
          onValidationFailed();
        return;
      }

    if(static_cast<bool>(m_certificateCache))
      {
        shared_ptr<const IdentityCertificate> trustedCert = m_certificateCache->getCertificate(keyLocatorName);
        if(static_cast<bool>(trustedCert)){
          if(Validator::verifySignature(buf, size, signature, trustedCert->getPublicKeyInfo()))
            onValidated();
          else
            onValidationFailed();
          return;
        }
      }
    
    OnDataValidated onKeyLocatorValidated = 
      bind(&ValidatorInvitation::onDskKeyLocatorValidated, 
           this, _1, buf, size, signature, onValidated, onValidationFailed);
    
    OnDataValidationFailed onKeyLocatorValidationFailed = 
      bind(&ValidatorInvitation::onDskKeyLocatorValidationFailed, 
           this, _1, onValidationFailed);
    
    Interest interest(keyLocatorName);
    interest.setMustBeFresh(true);
    
    shared_ptr<ValidationRequest> nextStep = make_shared<ValidationRequest>
      (interest, onKeyLocatorValidated, onKeyLocatorValidationFailed, 0, stepCount + 1);

    nextSteps.push_back(nextStep);
    return;
  }catch(...){
    onValidationFailed();
    return;
  }
}
 
void
ValidatorInvitation::processFinalSignature (const uint8_t* buf, 
                                            const size_t size,
                                            const SignatureSha256WithRsa& signature,
                                            const Name& keyLocatorName,
                                            const OnValidated& onValidated, 
                                            const OnValidationFailed& onValidationFailed)
{
  try{
    Name keyName = IdentityCertificate::certificateNameToPublicKeyName(keyLocatorName);

    if(m_trustAnchors.end() != m_trustAnchors.find(keyName) && Validator::verifySignature(buf, size, signature, m_trustAnchors[keyName]))
      onValidated();
    else
      onValidationFailed();
    return;
  }catch(...){
    onValidationFailed();
    return;
  }
}


void 
ValidatorInvitation::onDskKeyLocatorValidated(const shared_ptr<const Data>& certData, 
                                              const uint8_t* buf,
                                              const size_t size,
                                              const SignatureSha256WithRsa& signature,
                                              const OnValidated& onValidated, 
                                              const OnValidationFailed& onValidationFailed)
{
  shared_ptr<IdentityCertificate> certificate = make_shared<IdentityCertificate>(*certData);

  if(!certificate->isTooLate() && !certificate->isTooEarly())
    {
      Name certName = certificate->getName().getPrefix(-1);
      m_dskCertificates[certName] = certificate;

      if(Validator::verifySignature(buf, size, signature, certificate->getPublicKeyInfo()))
        {
          onValidated();
          return;
        }
    }

  onValidationFailed();
  return;
}

void
ValidatorInvitation::onDskKeyLocatorValidationFailed(const shared_ptr<const Data>& certData, 
                                                     const OnValidationFailed& onValidationFailed)
{ onValidationFailed(); }

}//chronos
