/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOS_VALIDATOR_PANEL_H
#define CHRONOS_VALIDATOR_PANEL_H

#include <ndn-cpp-dev/security/validator.hpp>
#include <ndn-cpp-dev/security/sec-rule-relative.hpp>
#include <ndn-cpp-dev/security/certificate-cache.hpp>
#include <map>

#include "endorse-certificate.h"

namespace chronos{

class ValidatorPanel : public ndn::Validator
{
public:
  
  static const ndn::shared_ptr<ndn::CertificateCache> DEFAULT_CERT_CACHE;

  ValidatorPanel(int stepLimit = 10,
                 const ndn::shared_ptr<ndn::CertificateCache> certificateCache = DEFAULT_CERT_CACHE);

  ~ValidatorPanel()
  {}
  
  inline void
  addTrustAnchor(const EndorseCertificate& selfEndorseCertificate);

  inline void
  removeTrustAnchor(const ndn::Name& keyName);

protected:
  virtual void
  checkPolicy (const ndn::Data& data, 
               int stepCount, 
               const ndn::OnDataValidated& onValidated, 
               const ndn::OnDataValidationFailed& onValidationFailed,
               std::vector<ndn::shared_ptr<ndn::ValidationRequest> >& nextSteps);

  virtual void
  checkPolicy (const ndn::Interest& interest, 
               int stepCount, 
               const ndn::OnInterestValidated& onValidated, 
               const ndn::OnInterestValidationFailed& onValidationFailed,
               std::vector<ndn::shared_ptr<ndn::ValidationRequest> >& nextSteps)
  {
    onValidationFailed(interest.shared_from_this(),
                       "No rules for interest.");
  }

private:
  int m_stepLimit;
  ndn::shared_ptr<ndn::CertificateCache> m_certificateCache;
  ndn::shared_ptr<ndn::SecRuleRelative> m_endorseeRule;
  std::map<ndn::Name, ndn::PublicKey> m_trustAnchors;
  
};

inline void
ValidatorPanel::addTrustAnchor(const EndorseCertificate& cert)
{ m_trustAnchors[cert.getPublicKeyName()] = cert.getPublicKeyInfo(); }

inline void 
ValidatorPanel::removeTrustAnchor(const ndn::Name& keyName)
{ m_trustAnchors.erase(keyName); }

}//chronos

#endif
