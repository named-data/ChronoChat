/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "validator-panel.h"

#include "logging.h"

using namespace std;
using namespace ndn;

INIT_LOGGER("ValidatorPanel");

namespace chronos{

using ndn::shared_ptr;

const shared_ptr<CertificateCache> ValidatorPanel::DEFAULT_CERT_CACHE = shared_ptr<CertificateCache>();

ValidatorPanel::ValidatorPanel(int stepLimit /* = 10 */,
                               const shared_ptr<CertificateCache> certificateCache/* = DEFAULT_CERT_CACHE */)
  : m_stepLimit(stepLimit)
  , m_certificateCache(certificateCache)
{
  m_endorseeRule = make_shared<SecRuleRelative>("^([^<DNS>]*)<DNS><>*<ENDORSEE><>$",
                                                "^([^<KEY>]*)<KEY>(<>*)<ksk-.*><ID-CERT>$",
                                                "==", "\\1", "\\1\\2", true);
}



void
ValidatorPanel::checkPolicy (const Data& data,
                             int stepCount,
                             const OnDataValidated& onValidated,
                             const OnDataValidationFailed& onValidationFailed,
                             std::vector<shared_ptr<ValidationRequest> >& nextSteps)
{
  if(m_stepLimit == stepCount)
    {
      _LOG_ERROR("Reach the maximum steps of verification!");
      onValidationFailed(data.shared_from_this(),
                         "Reach maximum validation steps: " + data.getName().toUri());
      return;
    }

  try
    {
      SignatureSha256WithRsa sig(data.getSignature());
      const Name& keyLocatorName = sig.getKeyLocator().getName();

      if(m_endorseeRule->satisfy(data.getName(), keyLocatorName))
        {
          Name keyName = IdentityCertificate::certificateNameToPublicKeyName(keyLocatorName);

          if(m_trustAnchors.end() != m_trustAnchors.find(keyName) && Validator::verifySignature(data, sig, m_trustAnchors[keyName]))
            onValidated(data.shared_from_this());
          else
            onValidationFailed(data.shared_from_this(), "Cannot verify signature:" + data.getName().toUri());
        }
      else
        onValidationFailed(data.shared_from_this(), "Does not satisfy rule: " + data.getName().toUri());

      return;
    }
  catch(SignatureSha256WithRsa::Error &e)
    {
      return onValidationFailed(data.shared_from_this(),
                                "Not SignatureSha256WithRsa signature: " + data.getName().toUri());
    }
  catch(KeyLocator::Error &e)
    {
      return onValidationFailed(data.shared_from_this(),
                                "Key Locator is not a name: " + data.getName().toUri());
    }
}

}//chronos
