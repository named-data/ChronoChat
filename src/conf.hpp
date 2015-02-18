/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_CONFIG_HPP
#define CHRONOCHAT_CONFIG_HPP

#include "common.hpp"
#include "tlv.hpp"
#include <ndn-cxx/util/concepts.hpp>
#include <ndn-cxx/encoding/block.hpp>
#include <ndn-cxx/encoding/encoding-buffer.hpp>
#include <list>

namespace chronochat {

class Conf
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

  Conf();

  explicit
  Conf(const Block& confWire);

  const Block&
  wireEncode() const;

  void
  wireDecode(const Block& confWire);

  const Name&
  getIdentity() const;

  const std::string&
  getNick() const;

  void
  setIdentity(const Name& identity);

  void
  setNick(const std::string& nick);

private:
  template<bool T>
  size_t
  wireEncode(ndn::EncodingImpl<T>& block) const;

private:
  mutable Block m_wire;
  Name m_identity;
  std::string m_nick;
};

inline const Name&
Conf::getIdentity() const
{
  return m_identity;
}

inline const std::string&
Conf::getNick() const
{
  return m_nick;
}

} // namespace chronochat

#endif //CHRONOCHAT_CONF_HPP
