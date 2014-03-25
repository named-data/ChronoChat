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
#include "invitation.h"

#include "logging.h"

using namespace ndn;

INIT_LOGGER("ValidatorInvitation");

namespace chronos{

using ndn::shared_ptr;

const shared_ptr<CertificateCache> ValidatorInvitation::DefaultCertificateCache = shared_ptr<CertificateCache>();

ValidatorInvitation::ValidatorInvitation()
  : Validator()
  , m_invitationReplyRule("^([^<CHRONOCHAT-INVITATION>]*)<CHRONOCHAT-INVITATION>", 
                          "^([^<KEY>]*)<KEY>(<>*)[<dsk-.*><ksk-.*>]<ID-CERT>$", 
                          "==", "\\1", "\\1\\2", true)
  , m_invitationInterestRule("^[^<CHRONOCHAT-INVITATION>]*<CHRONOCHAT-INVITATION><>{6}$")
  , m_innerKeyRegex("^([^<KEY>]*)<KEY>(<>*)[<dsk-.*><ksk-.*>]<ID-CERT><>$", "\\1\\2")
{
}

void
ValidatorInvitation::checkPolicy (const Data& data, 
                                  int stepCount, 
                                  const OnDataValidated& onValidated, 
                                  const OnDataValidationFailed& onValidationFailed,
                                  std::vector<shared_ptr<ValidationRequest> >& nextSteps)
{
  try
    {
      SignatureSha256WithRsa sig(data.getSignature());    
      const Name & keyLocatorName = sig.getKeyLocator().getName();
      
      if(!m_invitationReplyRule.satisfy(data.getName(), keyLocatorName))
        return onValidationFailed(data.shared_from_this(),
                                  "Does not comply with the invitation rule: "
                                  + data.getName().toUri() + " signed by: "
                                  + keyLocatorName.toUri());

      Data innerData;
      innerData.wireDecode(data.getContent().blockFromValue());
      
      return internalCheck(data.wireEncode().value(), 
                           data.wireEncode().value_size() - data.getSignature().getValue().size(),
                           sig,
                           innerData,
                           bind(onValidated, data.shared_from_this()), 
                           bind(onValidationFailed, data.shared_from_this(), _1));
    }
   catch(SignatureSha256WithRsa::Error &e)
     {
       return onValidationFailed(data.shared_from_this(), 
                                 "Not SignatureSha256WithRsa signature: " + data.getName().toUri());
     }
}

void
ValidatorInvitation::checkPolicy (const Interest& interest, 
                                  int stepCount, 
                                  const OnInterestValidated& onValidated, 
                                  const OnInterestValidationFailed& onValidationFailed,
                                  std::vector<shared_ptr<ValidationRequest> >& nextSteps)
{
  try
    {
      Name interestName  = interest.getName();
      
      if(!m_invitationInterestRule.match(interestName))
        return onValidationFailed(interest.shared_from_this(), 
                                  "Invalid interest name: " +  interest.getName().toUri());

      Name signedName = interestName.getPrefix(-1);
      Buffer signedBlob = Buffer(signedName.wireEncode().value(), signedName.wireEncode().value_size());

      Block signatureBlock = interestName.get(Invitation::SIGNATURE).blockFromValue();
      Block signatureInfo = interestName.get(Invitation::KEY_LOCATOR).blockFromValue();
      Signature signature(signatureInfo, signatureBlock);
      SignatureSha256WithRsa sig(signature);

      Data innerData;
      innerData.wireDecode(interestName.get(Invitation::INVITER_CERT).blockFromValue());

      return internalCheck(signedBlob.buf(), 
                           signedBlob.size(),
                           sig,
                           innerData,
                           bind(onValidated, interest.shared_from_this()), 
                           bind(onValidationFailed, interest.shared_from_this(), _1));
    }
  catch(SignatureSha256WithRsa::Error& e)
    {
      return onValidationFailed(interest.shared_from_this(), 
                                "Not SignatureSha256WithRsa signature: " + interest.getName().toUri());
    }
}

void
ValidatorInvitation::internalCheck(const uint8_t* buf, size_t size,
                                   const SignatureSha256WithRsa& sig,
                                   const Data& innerData,
                                   const OnValidated& onValidated, 
                                   const OnValidationFailed& onValidationFailed)
{
  try
    {
      const Name & keyLocatorName = sig.getKeyLocator().getName();
      Name signingKeyName = IdentityCertificate::certificateNameToPublicKeyName(keyLocatorName);
      
      if(m_trustAnchors.find(signingKeyName) == m_trustAnchors.end())
        return onValidationFailed("Cannot reach any trust anchor");

      if(!Validator::verifySignature(buf, size, sig, m_trustAnchors[signingKeyName]))
        return onValidationFailed("Cannot verify outer signature");

      // Temporarily disabled, we should get it back when we create a specific key for the chatroom.
      // if(!Validator::verifySignature(innerData, m_trustAnchors[signingKeyName]))
      //   return onValidationFailed("Cannot verify inner signature");

      if(!m_innerKeyRegex.match(innerData.getName())
          || m_innerKeyRegex.expand() != signingKeyName.getPrefix(-1))
         return onValidationFailed("Inner certificate does not comply with the rule");

      return onValidated();
    }
  catch(KeyLocator::Error& e)
    {
      return onValidationFailed("Key Locator is not a name");
    }
}


}//chronos
