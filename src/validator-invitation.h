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

#include <ndn-cpp-dev/security/validator.hpp>
#include <ndn-cpp-dev/security/certificate-cache.hpp>
#include <ndn-cpp-dev/security/sec-rule-relative.hpp>
#include <map>

#include "endorse-certificate.h"

namespace chronos{

class ValidatorInvitation : public ndn::Validator
{
  typedef ndn::function< void () > OnValidationFailed;
  typedef ndn::function< void () > OnValidated;

public:
  struct Error : public Validator::Error { Error(const std::string &what) : Validator::Error(what) {} };

  static const ndn::shared_ptr<ndn::CertificateCache> DefaultCertificateCache;

  ValidatorInvitation(ndn::shared_ptr<ndn::Face> face,                      
                      const std::string& chatroomName,
                      const ndn::Name& signingIdentity,
                      ndn::shared_ptr<ndn::CertificateCache> certificateCache = DefaultCertificateCache,
                      int stepLimit = 10);
  
  virtual
  ~ValidatorInvitation() {};

  void
  addTrustAnchor(const EndorseCertificate& cert)
  { m_trustAnchors[cert.getPublicKeyName()] = cert.getPublicKeyInfo(); }

  void
  removeTrustAnchor(const ndn::Name& keyName)
  { m_trustAnchors.erase(keyName); }

  ndn::shared_ptr<ndn::IdentityCertificate> 
  getValidatedDskCertificate(const ndn::Name& certName)
  {
    ValidatedCertifcates::iterator it = m_dskCertificates.find(certName);
    if(m_dskCertificates.end() != it)
      return it->second;
    else
      return ndn::shared_ptr<ndn::IdentityCertificate>();
  }


protected:
  void
  checkPolicy (const ndn::shared_ptr<const ndn::Data>& data, 
               int stepCount, 
               const ndn::OnDataValidated& onValidated, 
               const ndn::OnDataValidationFailed& onValidationFailed,
               std::vector<ndn::shared_ptr<ndn::ValidationRequest> >& nextSteps);

  void
  checkPolicy (const ndn::shared_ptr<const ndn::Interest>& interest, 
               int stepCount, 
               const ndn::OnInterestValidated& onValidated, 
               const ndn::OnInterestValidationFailed& onValidationFailed,
               std::vector<ndn::shared_ptr<ndn::ValidationRequest> >& nextSteps);

private:
  void 
  onDskKeyLocatorValidated(const ndn::shared_ptr<const ndn::Data>& certData, 
                           const uint8_t* buf,
                           const size_t size,
                           const ndn::SignatureSha256WithRsa& signature,
                           const OnValidated& onValidated, 
                           const OnValidationFailed& onValidationFailed);
  
  void
  onDskKeyLocatorValidationFailed(const ndn::shared_ptr<const ndn::Data>& certData, 
                                  const OnValidationFailed& onValidationFailed);

  void
  processSignature (const uint8_t* buf, 
                    const size_t size,
                    const ndn::SignatureSha256WithRsa& signature,
                    const ndn::Name& keyLocatorName,
                    const OnValidated& onValidated, 
                    const OnValidationFailed& onValidationFailed,
                    int stepCount,
                    std::vector<ndn::shared_ptr<ndn::ValidationRequest> >& nextSteps);

  void
  processFinalSignature (const uint8_t* buf, 
                         const size_t size,
                         const ndn::SignatureSha256WithRsa& signature,
                         const ndn::Name& keyLocatorName,
                         const OnValidated& onValidated, 
                         const OnValidationFailed& onValidationFailed);

private:

  typedef std::map<ndn::Name, ndn::PublicKey> TrustAnchors;
  typedef std::map<ndn::Name, ndn::shared_ptr<ndn::IdentityCertificate> > ValidatedCertifcates;

  int m_stepLimit;
  ndn::shared_ptr<ndn::CertificateCache> m_certificateCache;

  std::string m_chatroomName;
  ndn::Name m_signingIdentity;

  ndn::shared_ptr<ndn::SecRuleRelative> m_invitationRule;
  ndn::shared_ptr<ndn::SecRuleRelative> m_dskRule;

  ndn::shared_ptr<ndn::Regex> m_kskRegex;
  ndn::shared_ptr<ndn::Regex> m_keyNameRegex;

  TrustAnchors m_trustAnchors;

  ValidatedCertifcates m_dskCertificates;

};

}//chronos

#endif //CHRONOS_VALIDATOR_INVITATION_H
