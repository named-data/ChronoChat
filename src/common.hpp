/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * COPYRIGHT MSG GOES HERE...
 */

#ifndef CHRONOCHAT_COMMON_HPP
#define CHRONOCHAT_COMMON_HPP

#include "config.h"

#ifdef WITH_TESTS
#define CHRONOCHAT_VIRTUAL_WITH_TESTS virtual
#define CHRONOCHAT_PUBLIC_WITH_TESTS_ELSE_PROTECTED public
#define CHRONOCHAT_PUBLIC_WITH_TESTS_ELSE_PRIVATE public
#define CHRONOCHAT_PROTECTED_WITH_TESTS_ELSE_PRIVATE protected
#else
#define CHRONOCHAT_VIRTUAL_WITH_TESTS
#define CHRONOCHAT_PUBLIC_WITH_TESTS_ELSE_PROTECTED protected
#define CHRONOCHAT_PUBLIC_WITH_TESTS_ELSE_PRIVATE private
#define CHRONOCHAT_PROTECTED_WITH_TESTS_ELSE_PRIVATE private
#endif

#include <cstddef>
#include <list>
#include <set>
#include <map>
#include <queue>
#include <vector>
#include <string>

#include <ndn-cxx/common.hpp>
#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/data.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/property_tree/ptree.hpp>

namespace chronochat {

using std::size_t;

using boost::noncopyable;

using std::shared_ptr;
using std::unique_ptr;
using std::weak_ptr;
using std::enable_shared_from_this;
using std::make_shared;
using std::static_pointer_cast;
using std::dynamic_pointer_cast;
using std::const_pointer_cast;
using std::function;
using std::bind;
using std::ref;
using std::cref;

using ndn::Interest;
using ndn::Data;
using ndn::Name;
using ndn::Exclude;
using ndn::Block;
using ndn::Signature;
using ndn::KeyLocator;

namespace tlv {
using namespace ndn::tlv;
}

namespace name = ndn::name;
namespace time = ndn::time;

} // namespace chronochat

#endif // CHRONOCHAT_COMMON_HPP
