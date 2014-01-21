/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef SEC_POLICY_CHRONO_CHAT_INVITATION_H
#define SEC_POLICY_CHRONO_CHAT_INVITATION_H

#include <ndn-cpp-dev/security/sec-policy.hpp>
#include <ndn-cpp-et/policy/sec-rule-relative.hpp>
#include <ndn-cpp-et/policy/sec-rule-specific.hpp>
#include <ndn-cpp-et/cache/ttl-certificate-cache.hpp>
#include <ndn-cpp-et/regex/regex.hpp>
#include <map>

#include "endorse-certificate.h"

class SecPolicyChronoChatInvitation : public ndn::SecPolicy
{
public:
  SecPolicyChronoChatInvitation(const std::string& chatroomName,
                                const ndn::Name& signingIdentity,
                                int stepLimit = 10);
  
  virtual
  ~SecPolicyChronoChatInvitation();

  bool 
  skipVerifyAndTrust (const ndn::Data& data);

  bool
  requireVerify (const ndn::Data& data);

  ndn::ptr_lib::shared_ptr<ndn::ValidationRequest>
  checkVerificationPolicy(const ndn::ptr_lib::shared_ptr<ndn::Data>& data, 
                          int stepCount, 
                          const ndn::OnVerified& onVerified,
                          const ndn::OnVerifyFailed& onVerifyFailed);

  bool 
  checkSigningPolicy(const ndn::Name& dataName, 
                     const ndn::Name& certificateName);
    
  ndn::Name 
  inferSigningIdentity(const ndn::Name& dataName);

  void
  addTrustAnchor(const EndorseCertificate& selfEndorseCertificate);
  
  // void 
  // addChatDataRule(const ndn::Name& prefix, 
  //                 const ndn::security::IdentityCertificate identityCertificate);

  ndn::ptr_lib::shared_ptr<ndn::IdentityCertificate> 
  getValidatedDskCertificate(const ndn::Name& certName);

private:
  void 
  onDskCertificateVerified(const ndn::ptr_lib::shared_ptr<ndn::Data>& certData, 
                           ndn::ptr_lib::shared_ptr<ndn::Data> originalData,
                           const ndn::OnVerified& onVerified, 
                           const ndn::OnVerifyFailed& onVerifyFailed);

  void
  onDskCertificateVerifyFailed(const ndn::ptr_lib::shared_ptr<ndn::Data>& certData, 
                               ndn::ptr_lib::shared_ptr<ndn::Data> originalData,
                               const ndn::OnVerifyFailed& onVerifyFailed);

private:
  std::string m_chatroomName;
  ndn::Name m_signingIdentity;

  int m_stepLimit;

  ndn::TTLCertificateCache m_certificateCache;

  ndn::ptr_lib::shared_ptr<ndn::SecRuleRelative> m_invitationPolicyRule;
  ndn::ptr_lib::shared_ptr<ndn::SecRuleRelative> m_dskRule;
  std::map<ndn::Name, ndn::SecRuleSpecific> m_chatDataRules;

  ndn::ptr_lib::shared_ptr<ndn::Regex> m_kskRegex;
  ndn::ptr_lib::shared_ptr<ndn::Regex> m_keyNameRegex;

  std::map<ndn::Name, ndn::PublicKey> m_trustAnchors;

  std::map<ndn::Name, ndn::ptr_lib::shared_ptr<ndn::IdentityCertificate> > m_dskCertificates;

};

#endif //CHATROOM_POLICY_MANAGER_H
