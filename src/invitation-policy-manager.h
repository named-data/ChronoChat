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

#include <ndn-cpp/security/policy/policy-manager.hpp>
#include <ndn-cpp-et/policy-manager/identity-policy-rule.hpp>
#include <ndn-cpp-et/cache/ttl-certificate-cache.hpp>
#include <ndn-cpp-et/regex/regex.hpp>
#include <map>

#include "endorse-certificate.h"
#include "chat-policy-rule.h"

class InvitationPolicyManager : public ndn::PolicyManager
{
public:
  InvitationPolicyManager(const std::string& chatroomName,
                          const ndn::Name& signingIdentity,
                          int stepLimit = 10);
  
  virtual
  ~InvitationPolicyManager();

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

  ndn::ptr_lib::shared_ptr<ndn::IdentityPolicyRule> m_invitationPolicyRule;
  ndn::ptr_lib::shared_ptr<ndn::IdentityPolicyRule> m_dskRule;
  std::map<ndn::Name, ChatPolicyRule, ndn::Name::BreadthFirstLess> m_chatDataRules;

  ndn::ptr_lib::shared_ptr<ndn::Regex> m_kskRegex;
  ndn::ptr_lib::shared_ptr<ndn::Regex> m_keyNameRegex;

  std::map<ndn::Name, ndn::PublicKey, ndn::Name::BreadthFirstLess> m_trustAnchors;

  std::map<ndn::Name, ndn::ptr_lib::shared_ptr<ndn::IdentityCertificate>, ndn::Name::BreadthFirstLess> m_dskCertificates;

};

#endif //CHATROOM_POLICY_MANAGER_H
