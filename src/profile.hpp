/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 *         Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_PROFILE_HPP
#define CHRONOCHAT_PROFILE_HPP

#include "common.hpp"

#include "tlv.hpp"
#include <ndn-cxx/util/concepts.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/encoding-buffer.hpp>
#include <ndn-cxx/security/certificate.hpp>

namespace chronochat {

class Profile
{

public:
  class Error : public std::runtime_error
  {
  public:
    explicit
    Error(const std::string& what)
      : std::runtime_error(what)
    {
    }
  };

public:
  typedef std::map<std::string, std::string>::iterator iterator;
  typedef std::map<std::string, std::string>::const_iterator const_iterator;

  Profile()
  {
  }

  Profile(const ndn::security::Certificate& identityCertificate);

  Profile(const Name& identityName);

  Profile(const Name& identityName,
          const std::string& name,
          const std::string& institution);

  Profile(const Profile& profile);

  Profile(const Block& profileWire);

  ~Profile()
  {
  }

  const Block&
  wireEncode() const;

  void
  wireDecode(const Block& profileWire);

  std::string&
  operator[](const std::string& profileKey)
  {
    return m_entries[profileKey];
  }

  std::string
  get(const std::string& profileKey) const
  {
    std::map<std::string, std::string>::const_iterator it = m_entries.find(profileKey);
    if (it != m_entries.end())
      return it->second;
    else
      return std::string();
  }

  Profile::iterator
  begin()
  {
    return m_entries.begin();
  }

  Profile::const_iterator
  begin() const
  {
    return m_entries.begin();
  }

  Profile::iterator
  end()
  {
    return m_entries.end();
  }

  Profile::const_iterator
  end() const
  {
    return m_entries.end();
  }

  Name
  getIdentityName() const
  {
    return ndn::Name(m_entries.at("IDENTITY"));
  }

  bool
  operator==(const Profile& profile) const;

  bool
  operator!=(const Profile& profile) const;

private:
  template<ndn::encoding::Tag T>
  size_t
  wireEncode(ndn::EncodingImpl<T>& block) const;


private:
  static const std::string OID_NAME;
  static const std::string OID_ORG;
  static const std::string OID_GROUP;
  static const std::string OID_HOMEPAGE;
  static const std::string OID_ADVISOR;
  static const std::string OID_EMAIL;

  mutable Block m_wire;

  std::map<std::string, std::string> m_entries;
};

} // namespace chronochat

#endif // CHRONOCHAT_PROFILE_HPP
