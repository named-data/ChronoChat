/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef INVITATION_POLICY_MANAGER_H
#define INVITATION_POLICY_MANAGER_H

#include <ndn.cxx/security/policy/policy-manager.h>
#include <ndn.cxx/security/policy/identity-policy-rule.h>
#include <ndn.cxx/security/cache/certificate-cache.h>
#include <map>

#include "endorse-certificate.h"

class InvitationPolicyManager : public ndn::security::PolicyManager
{
public:
  InvitationPolicyManager(const int & stepLimit,                        
                          ndn::Ptr<ndn::security::CertificateCache> certificateCache);

  ~InvitationPolicyManager()
  {}

  /**
   * @brief check if the received data packet can escape from verification
   * @param data the received data packet
   * @return true if the data does not need to be verified, otherwise false
   */
  bool 
  skipVerifyAndTrust (const ndn::Data & data);

  /**
   * @brief check if PolicyManager has the verification rule for the received data
   * @param data the received data packet
   * @return true if the data must be verified, otherwise false
   */
  bool
  requireVerify (const ndn::Data & data);

  /**
   * @brief check whether received data packet complies with the verification policy, and get the indication of next verification step
   * @param data the received data packet
   * @param stepCount the number of verification steps that have been done, used to track the verification progress
   * @param verifiedCallback the callback function that will be called if the received data packet has been validated
   * @param unverifiedCallback the callback function that will be called if the received data packet cannot be validated
   * @return the indication of next verification step, NULL if there is no further step
   */
  ndn::Ptr<ndn::security::ValidationRequest>
  checkVerificationPolicy(ndn::Ptr<ndn::Data> data, 
                          const int & stepCount, 
                          const ndn::DataCallback& verifiedCallback,
                          const ndn::UnverifiedCallback& unverifiedCallback);

    
  /**
   * @brief check if the signing certificate name and data name satify the signing policy 
   * @param dataName the name of data to be signed
   * @param certificateName the name of signing certificate
   * @return true if the signing certificate can be used to sign the data, otherwise false
   */
  bool 
  checkSigningPolicy(const ndn::Name & dataName, const ndn::Name & certificateName);
  
  /**
   * @brief Infer signing identity name according to policy, if the signing identity cannot be inferred, it should return empty name
   * @param dataName, the name of data to be signed
   * @return the signing identity. 
   */
  ndn::Name 
  inferSigningIdentity(const ndn::Name & dataName);

  
  void
  addTrustAnchor(const EndorseCertificate& selfEndorseCertificate);

private:
  void 
  onCertificateVerified(ndn::Ptr<ndn::Data> certData, 
                        ndn::Ptr<ndn::Data> originalData,
                        const ndn::DataCallback& verifiedCallback, 
                        const ndn::UnverifiedCallback& unverifiedCallback);

  void
  onCertificateUnverified(ndn::Ptr<ndn::Data> certData, 
                          ndn::Ptr<ndn::Data> originalData,
                          const ndn::UnverifiedCallback& unverifiedCallback);

private:
  int m_stepLimit;
  ndn::Ptr<ndn::security::CertificateCache> m_certificateCache;
  ndn::Ptr<ndn::Regex> m_localPrefixRegex;
  ndn::Ptr<ndn::security::IdentityPolicyRule> m_invitationDataRule;
  ndn::Ptr<ndn::security::IdentityPolicyRule> m_dskRule;
  ndn::Ptr<ndn::Regex> m_keyNameRegex;
  ndn::Ptr<ndn::Regex> m_signingCertificateRegex;
  std::map<ndn::Name, ndn::security::Publickey> m_trustAnchors;
  
};

#endif
