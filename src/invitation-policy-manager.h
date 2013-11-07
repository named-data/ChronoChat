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
#include <ndn.cxx/regex/regex.h>
#include <map>

#include "endorse-certificate.h"
#include "chat-policy-rule.h"

class InvitationPolicyManager : public ndn::security::PolicyManager
{
public:
  InvitationPolicyManager(const std::string& chatroomName,
                          const ndn::Name& signingIdentity,
                          int stepLimit = 10,
                          ndn::Ptr<ndn::security::CertificateCache> certificateCache = NULL);
  
  virtual
  ~InvitationPolicyManager();

  bool 
  skipVerifyAndTrust (const ndn::Data& data);

  bool
  requireVerify (const ndn::Data& data);

  ndn::Ptr<ndn::security::ValidationRequest>
  checkVerificationPolicy(ndn::Ptr<ndn::Data> data, 
                          const int& stepCount, 
                          const ndn::DataCallback& verifiedCallback,
                          const ndn::UnverifiedCallback& unverifiedCallback);

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

  ndn::Ptr<ndn::security::IdentityCertificate> 
  getValidatedDskCertificate(const ndn::Name& certName);

private:
  void 
  onDskCertificateVerified(ndn::Ptr<ndn::Data> certData, 
                        ndn::Ptr<ndn::Data> originalData,
                        const ndn::DataCallback& verifiedCallback, 
                        const ndn::UnverifiedCallback& unverifiedCallback);

  void
  onDskCertificateUnverified(ndn::Ptr<ndn::Data> certData, 
                          ndn::Ptr<ndn::Data> originalData,
                          const ndn::UnverifiedCallback& unverifiedCallback);

private:
  std::string m_chatroomName;
  ndn::Name m_signingIdentity;

  int m_stepLimit;

  ndn::Ptr<ndn::security::CertificateCache> m_certificateCache;

  ndn::Ptr<ndn::security::IdentityPolicyRule> m_invitationPolicyRule;
  ndn::Ptr<ndn::security::IdentityPolicyRule> m_dskRule;
  std::map<ndn::Name, ChatPolicyRule> m_chatDataRules;

  ndn::Ptr<ndn::Regex> m_kskRegex;
  ndn::Ptr<ndn::Regex> m_keyNameRegex;

  std::map<ndn::Name, ndn::security::Publickey> m_trustAnchors;

  std::map<ndn::Name, ndn::Ptr<ndn::security::IdentityCertificate> > m_dskCertificates;

};

#endif //CHATROOM_POLICY_MANAGER_H
