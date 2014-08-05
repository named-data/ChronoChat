/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOS_VALIDATOR_INVITATION_HPP
#define CHRONOS_VALIDATOR_INVITATION_HPP

#include "common.hpp"

#include <ndn-cxx/security/validator.hpp>
#include <ndn-cxx/security/certificate-cache.hpp>
#include <ndn-cxx/security/sec-rule-relative.hpp>

#include "endorse-certificate.hpp"

namespace chronos {

class ValidatorInvitation : public ndn::Validator
{
  typedef function<void(const std::string&)> OnValidationFailed;
  typedef function<void()> OnValidated;

public:
  class Error : public ndn::Validator::Error
  {
  public:
    Error(const std::string& what)
      : ndn::Validator::Error(what)
    {
    }
  };

  static const shared_ptr<ndn::CertificateCache> DefaultCertificateCache;

  ValidatorInvitation();

  virtual
  ~ValidatorInvitation()
  {
  }

  void
  addTrustAnchor(const Name& keyName, const ndn::PublicKey& key);

  void
  removeTrustAnchor(const Name& keyName);

  void
  cleanTrustAnchor();

protected:
  void
  checkPolicy(const Data& data,
              int stepCount,
              const ndn::OnDataValidated& onValidated,
              const ndn::OnDataValidationFailed& onValidationFailed,
              std::vector<shared_ptr<ndn::ValidationRequest> >& nextSteps);

  void
  checkPolicy(const Interest& interest,
              int stepCount,
              const ndn::OnInterestValidated& onValidated,
              const ndn::OnInterestValidationFailed& onValidationFailed,
              std::vector<shared_ptr<ndn::ValidationRequest> >& nextSteps);

private:
  void
  internalCheck(const uint8_t* buf, size_t size,
                const Signature& sig,
                const Name& keyLocatorName,
                const Data& innerData,
                const OnValidated& onValidated,
                const OnValidationFailed& onValidationFailed);

private:
  typedef std::map<Name, ndn::PublicKey> TrustAnchors;

  ndn::SecRuleRelative m_invitationReplyRule;
  ndn::Regex m_invitationInterestRule;
  ndn::Regex m_innerKeyRegex;
  TrustAnchors m_trustAnchors;
};

} // namespace chronos

#endif //CHRONOS_VALIDATOR_INVITATION_HPP
