/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "validator-panel.hpp"

#include "logging.h"

namespace chronochat {

using std::vector;

using ndn::CertificateCache;
using ndn::SecRuleRelative;
using ndn::OnDataValidated;
using ndn::OnDataValidationFailed;
using ndn::ValidationRequest;
using ndn::IdentityCertificate;

const shared_ptr<CertificateCache> ValidatorPanel::DEFAULT_CERT_CACHE =
  shared_ptr<CertificateCache>();

ValidatorPanel::ValidatorPanel(int stepLimit,
                               const shared_ptr<CertificateCache> certificateCache)
  : m_stepLimit(stepLimit)
  , m_certificateCache(certificateCache)
{
  m_endorseeRule = make_shared<SecRuleRelative>("^([^<DNS>]*)<DNS><>*<ENDORSEE><>$",
                                                "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$",
                                                "==", "\\1", "\\1\\2", true);
}

void
ValidatorPanel::addTrustAnchor(const EndorseCertificate& cert)
{
  m_trustAnchors[cert.getPublicKeyName()] = cert.getPublicKeyInfo();
}

void
ValidatorPanel::removeTrustAnchor(const Name& keyName)
{
  m_trustAnchors.erase(keyName);
}

void
ValidatorPanel::checkPolicy (const Data& data,
                             int stepCount,
                             const OnDataValidated& onValidated,
                             const OnDataValidationFailed& onValidationFailed,
                             vector<shared_ptr<ValidationRequest> >& nextSteps)
{
  if (m_stepLimit == stepCount) {
    onValidationFailed(data.shared_from_this(),
                       "Reach maximum validation steps: " + data.getName().toUri());
    return;
  }

  const KeyLocator& keyLocator = data.getSignature().getKeyLocator();

  if (keyLocator.getType() != KeyLocator::KeyLocator_Name)
    return onValidationFailed(data.shared_from_this(),
                              "Key Locator is not a name: " + data.getName().toUri());

  const Name& keyLocatorName = keyLocator.getName();

  if (m_endorseeRule->satisfy(data.getName(), keyLocatorName)) {
    Name keyName = IdentityCertificate::certificateNameToPublicKeyName(keyLocatorName);

    if (m_trustAnchors.end() != m_trustAnchors.find(keyName) &&
        Validator::verifySignature(data, data.getSignature(), m_trustAnchors[keyName]))
      onValidated(data.shared_from_this());
    else
      onValidationFailed(data.shared_from_this(),
                         "Cannot verify signature:" + data.getName().toUri());
  }
  else
    onValidationFailed(data.shared_from_this(),
                       "Does not satisfy rule: " + data.getName().toUri());

  return;
}

} // namespace chronochat
