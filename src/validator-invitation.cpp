/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "validator-invitation.hpp"
#include "invitation.hpp"

#include "logging.h"

namespace chronochat {

using std::vector;

using ndn::CertificateCache;
using ndn::SecRuleRelative;
using ndn::OnDataValidated;
using ndn::OnDataValidationFailed;
using ndn::OnInterestValidated;
using ndn::OnInterestValidationFailed;
using ndn::ValidationRequest;
using ndn::IdentityCertificate;

const shared_ptr<CertificateCache> ValidatorInvitation::DefaultCertificateCache =
  shared_ptr<CertificateCache>();

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
ValidatorInvitation::addTrustAnchor(const Name& keyName, const ndn::PublicKey& key)
{
  m_trustAnchors[keyName] = key;
}

void
ValidatorInvitation::removeTrustAnchor(const Name& keyName)
{
  m_trustAnchors.erase(keyName);
}

void
ValidatorInvitation::cleanTrustAnchor()
{
  m_trustAnchors.clear();
}

void
ValidatorInvitation::checkPolicy (const Data& data,
                                  int stepCount,
                                  const OnDataValidated& onValidated,
                                  const OnDataValidationFailed& onValidationFailed,
                                  vector<shared_ptr<ValidationRequest> >& nextSteps)
{
  const Signature& signature = data.getSignature();

  if (signature.getKeyLocator().getType() != KeyLocator::KeyLocator_Name)
    return onValidationFailed(data.shared_from_this(),
                              "Key Locator is not a name: " + data.getName().toUri());

  const Name & keyLocatorName = signature.getKeyLocator().getName();

  if (!m_invitationReplyRule.satisfy(data.getName(), keyLocatorName))
    return onValidationFailed(data.shared_from_this(),
                              "Does not comply with the invitation rule: " +
                              data.getName().toUri() + " signed by: " +
                              keyLocatorName.toUri());

  Data innerData;
  innerData.wireDecode(data.getContent().blockFromValue());

  return internalCheck(data.wireEncode().value(),
                       data.wireEncode().value_size() - data.getSignature().getValue().size(),
                       signature,
                       keyLocatorName,
                       innerData,
                       bind(onValidated, data.shared_from_this()),
                       bind(onValidationFailed, data.shared_from_this(), _1));
}

void
ValidatorInvitation::checkPolicy (const Interest& interest,
                                  int stepCount,
                                  const OnInterestValidated& onValidated,
                                  const OnInterestValidationFailed& onValidationFailed,
                                  vector<shared_ptr<ValidationRequest> >& nextSteps)
{
  const Name& interestName  = interest.getName();

  if (!m_invitationInterestRule.match(interestName))
    return onValidationFailed(interest.shared_from_this(),
                              "Invalid interest name: " +  interest.getName().toUri());

  Name signedName = interestName.getPrefix(-1);
  ndn::Buffer signedBlob = ndn::Buffer(signedName.wireEncode().value(),
                                       signedName.wireEncode().value_size());

  Block signatureBlock = interestName.get(Invitation::SIGNATURE).blockFromValue();
  Block signatureInfo = interestName.get(Invitation::KEY_LOCATOR).blockFromValue();
  Signature signature(signatureInfo, signatureBlock);

  if (signature.getKeyLocator().getType() != KeyLocator::KeyLocator_Name)
    return onValidationFailed(interest.shared_from_this(),
                              "KeyLocator is not a name: " + interest.getName().toUri());

  const Name & keyLocatorName = signature.getKeyLocator().getName();

  Data innerData;
  innerData.wireDecode(interestName.get(Invitation::INVITER_CERT).blockFromValue());

  return internalCheck(signedBlob.buf(), signedBlob.size(),
                       signature,
                       keyLocatorName,
                       innerData,
                       bind(onValidated, interest.shared_from_this()),
                       bind(onValidationFailed, interest.shared_from_this(), _1));
}

void
ValidatorInvitation::internalCheck(const uint8_t* buf, size_t size,
                                   const Signature& signature,
                                   const Name& keyLocatorName,
                                   const Data& innerData,
                                   const OnValidated& onValidated,
                                   const OnValidationFailed& onValidationFailed)
{
  Name signingKeyName = IdentityCertificate::certificateNameToPublicKeyName(keyLocatorName);

  TrustAnchors::const_iterator keyIt = m_trustAnchors.find(signingKeyName);
  if (keyIt == m_trustAnchors.end())
    return onValidationFailed("Cannot reach any trust anchor");

  if (!Validator::verifySignature(buf, size, signature, keyIt->second))
    return onValidationFailed("Cannot verify outer signature");

  // Temporarily disabled, we should get it back when we create a specific key for the chatroom.
  // if(!Validator::verifySignature(innerData, m_trustAnchors[signingKeyName]))
  //   return onValidationFailed("Cannot verify inner signature");

  if (!m_innerKeyRegex.match(innerData.getName()) ||
      m_innerKeyRegex.expand() != signingKeyName.getPrefix(-1))
    return onValidationFailed("Inner certificate does not comply with the rule");

  return onValidated();
}

} // namespace chronochat
