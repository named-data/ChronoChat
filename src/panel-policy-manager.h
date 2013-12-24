/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef PANEL_POLICY_MANAGER_H
#define PANEL_POLICY_MANAGER_H

#include <ndn-cpp/security/policy/policy-manager.hpp>
#include <ndn-cpp-et/policy-manager/identity-policy-rule.hpp>
#include <ndn-cpp-et/cache/ttl-certificate-cache.hpp>
#include <map>

#include "endorse-certificate.h"

class PanelPolicyManager : public ndn::PolicyManager
{
public:
  PanelPolicyManager(const int & stepLimit = 10);

  ~PanelPolicyManager()
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
  ndn::ptr_lib::shared_ptr<ndn::ValidationRequest>
  checkVerificationPolicy(const ndn::ptr_lib::shared_ptr<ndn::Data>& data, 
                          int stepCount, 
                          const ndn::OnVerified& onVerified,
                          const ndn::OnVerifyFailed& onVerifyFailed);

    
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

  void
  removeTrustAnchor(const ndn::Name& keyName);

  ndn::ptr_lib::shared_ptr<ndn::PublicKey>
  getTrustedKey(const ndn::Name& inviterCertName);

private:
  static bool
  isSameKey(const ndn::Blob& keyA, const ndn::Blob& keyB);

private:
  int m_stepLimit;
  ndn::TTLCertificateCache m_certificateCache;
  ndn::ptr_lib::shared_ptr<ndn::Regex> m_localPrefixRegex;
  ndn::ptr_lib::shared_ptr<ndn::IdentityPolicyRule> m_invitationDataSigningRule;
  ndn::ptr_lib::shared_ptr<ndn::Regex> m_kskRegex;
  ndn::ptr_lib::shared_ptr<ndn::IdentityPolicyRule> m_dskRule;
  ndn::ptr_lib::shared_ptr<ndn::IdentityPolicyRule> m_endorseeRule;
  ndn::ptr_lib::shared_ptr<ndn::Regex> m_keyNameRegex;
  ndn::ptr_lib::shared_ptr<ndn::Regex> m_signingCertificateRegex;
  std::map<ndn::Name, ndn::PublicKey, ndn::Name::BreadthFirstLess> m_trustAnchors;
  
};

#endif
