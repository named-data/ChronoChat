/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOS_VALIDATOR_INVITATION_H
#define CHRONOS_VALIDATOR_INVITATION_H

#include <ndn-cxx/security/validator.hpp>
#include <ndn-cxx/security/certificate-cache.hpp>
#include <ndn-cxx/security/sec-rule-relative.hpp>
#include <map>

#include "endorse-certificate.h"

namespace chronos{

class ValidatorInvitation : public ndn::Validator
{
  typedef ndn::function< void (const std::string&) > OnValidationFailed;
  typedef ndn::function< void () > OnValidated;

public:
  struct Error : public Validator::Error { Error(const std::string &what) : Validator::Error(what) {} };

  static const ndn::shared_ptr<ndn::CertificateCache> DefaultCertificateCache;

  ValidatorInvitation();

  virtual
  ~ValidatorInvitation() {};

  void
  addTrustAnchor(const ndn::Name& keyName, const ndn::PublicKey& key)
  {
    m_trustAnchors[keyName] = key;
  }

  void
  removeTrustAnchor(const ndn::Name& keyName)
  {
    m_trustAnchors.erase(keyName);
  }

  void
  cleanTrustAnchor()
  {
    m_trustAnchors.clear();
  }

protected:
  void
  checkPolicy(const ndn::Data& data,
              int stepCount,
              const ndn::OnDataValidated& onValidated,
              const ndn::OnDataValidationFailed& onValidationFailed,
              std::vector<ndn::shared_ptr<ndn::ValidationRequest> >& nextSteps);

  void
  checkPolicy(const ndn::Interest& interest,
              int stepCount,
              const ndn::OnInterestValidated& onValidated,
              const ndn::OnInterestValidationFailed& onValidationFailed,
              std::vector<ndn::shared_ptr<ndn::ValidationRequest> >& nextSteps);

private:
  void
  internalCheck(const uint8_t* buf, size_t size,
                const ndn::SignatureSha256WithRsa& sig,
                const ndn::Data& innerData,
                const OnValidated& onValidated,
                const OnValidationFailed& onValidationFailed);

private:
  typedef std::map<ndn::Name, ndn::PublicKey> TrustAnchors;

  ndn::SecRuleRelative m_invitationReplyRule;
  ndn::Regex m_invitationInterestRule;
  ndn::Regex m_innerKeyRegex;
  TrustAnchors m_trustAnchors;
};

}//chronos

#endif //CHRONOS_VALIDATOR_INVITATION_H
