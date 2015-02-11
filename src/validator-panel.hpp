/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_VALIDATOR_PANEL_HPP
#define CHRONOCHAT_VALIDATOR_PANEL_HPP

#include "common.hpp"

#include <ndn-cxx/security/validator.hpp>
#include <ndn-cxx/security/sec-rule-relative.hpp>
#include <ndn-cxx/security/certificate-cache.hpp>

#include "endorse-certificate.hpp"

namespace chronochat {

class ValidatorPanel : public ndn::Validator
{
public:

  static const shared_ptr<ndn::CertificateCache> DEFAULT_CERT_CACHE;

  ValidatorPanel(int stepLimit = 10,
                 const shared_ptr<ndn::CertificateCache> cache = DEFAULT_CERT_CACHE);

  ~ValidatorPanel()
  {
  }

  void
  addTrustAnchor(const EndorseCertificate& selfEndorseCertificate);

  void
  removeTrustAnchor(const Name& keyName);

protected:
  virtual void
  checkPolicy(const Data& data,
              int stepCount,
              const ndn::OnDataValidated& onValidated,
              const ndn::OnDataValidationFailed& onValidationFailed,
              std::vector<shared_ptr<ndn::ValidationRequest> >& nextSteps);

  virtual void
  checkPolicy(const Interest& interest,
              int stepCount,
              const ndn::OnInterestValidated& onValidated,
              const ndn::OnInterestValidationFailed& onValidationFailed,
              std::vector<shared_ptr<ndn::ValidationRequest> >& nextSteps)
  {
    onValidationFailed(interest.shared_from_this(),
                       "No rules for interest.");
  }

private:
  int m_stepLimit;
  shared_ptr<ndn::CertificateCache> m_certificateCache;
  shared_ptr<ndn::SecRuleRelative> m_endorseeRule;
  std::map<Name, ndn::PublicKey> m_trustAnchors;
};

} // namespace chronochat

#endif // CHRONOCHAT_VALIDATOR_PANEL_HPP
