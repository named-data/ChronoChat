/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_TRUST_TREE_NODE_HPP
#define CHRONOCHAT_TRUST_TREE_NODE_HPP

#include "common.hpp"

namespace chronochat {

class TrustTreeNode;

typedef std::vector<shared_ptr<TrustTreeNode> > TrustTreeNodeList;

class TrustTreeNode
{
public:
  TrustTreeNode()
    : m_level(-1)
    , m_visited(false)
  {
  }

  TrustTreeNode(const Name& name)
    : m_name(name)
    , m_level(-1)
    , m_visited(false)
  {
  }

  ~TrustTreeNode()
  {
  }

  const Name&
  name()
  {
    return m_name;
  }

  void
  addIntroducee(const shared_ptr<TrustTreeNode>& introducee)
  {
    m_introducees.push_back(introducee);
  }

  TrustTreeNodeList&
  getIntroducees()
  {
    return m_introducees;
  }

  void
  addIntroducer(const shared_ptr<TrustTreeNode>& introducer)
  {
    m_introducers.push_back(introducer);
  }

  TrustTreeNodeList&
  getIntroducers()
  {
    return m_introducers;
  }

  void
  setLevel(int level)
  {
    m_level = level;
  }

  int
  level()
  {
    return m_level;
  }

  void
  setVisited()
  {
    m_visited = true;
  }

  void
  resetVisited()
  {
    m_visited = false;
  }

  bool
  visited()
  {
    return m_visited;
  }

public:
  double x;
  double y;

private:
  Name m_name;
  TrustTreeNodeList m_introducees;
  TrustTreeNodeList m_introducers;
  int m_level;
  bool m_visited;
};

} // namespace chronochat

#endif // CHRONOCHAT_TRUST_TREE_NODE_HPP
