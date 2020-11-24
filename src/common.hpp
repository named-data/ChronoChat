/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2020, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Mengjin Yan <jane.yan0129@gmail.com>
 *         Yingdi Yu <yingdi@cs.ucla.edu>
 *         Qiuhan Ding <qiuhanding@cs.ucla.edu>
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
#include <map>
#include <vector>
#include <string>

#include <boost/assert.hpp>

#include <ndn-cxx/interest.hpp>
#include <ndn-cxx/data.hpp>

namespace chronochat {

using std::size_t;

using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;
using std::function;
using std::bind;

using ndn::Interest;
using ndn::Data;
using ndn::Name;
using ndn::Block;

namespace tlv {
using namespace ndn::tlv;
} // namespace tlv

namespace name = ndn::name;
namespace time = ndn::time;

} // namespace chronochat

#endif // CHRONOCHAT_COMMON_HPP
