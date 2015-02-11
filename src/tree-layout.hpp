/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Zhenkai Zhu <zhenkai@cs.ucla.edu>
 *         Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *         Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_TREE_LAYOUT_HPP
#define CHRONOCHAT_TREE_LAYOUT_HPP

#include "trust-tree-node.hpp"

namespace chronochat {

class TreeLayout
{
public:
  struct Coordinate
  {
    double x;
    double y;
  };

  TreeLayout()
  {
  }

  virtual
  ~TreeLayout()
  {
  }

  virtual void
  setOneLevelLayout(std::vector<Coordinate>& childNodesCo)
  {
  }

  void
  setSiblingDistance(int d)
  {
    m_siblingDistance = d;
  }

  void
  setLevelDistance(int d)
  {
    m_levelDistance = d;
  }

  int
  getSiblingDistance()
  {
    return m_siblingDistance;
  }

  int
  getLevelDistance()
  {
    return m_levelDistance;
  }


private:
  int m_siblingDistance;
  int m_levelDistance;
};

class OneLevelTreeLayout : public TreeLayout
{
public:
  OneLevelTreeLayout()
  {
  }

  virtual ~OneLevelTreeLayout()
  {
  }

  virtual void
  setOneLevelLayout(std::vector<Coordinate>& childNodesCo);

};

class MultipleLevelTreeLayout : public TreeLayout
{
public:
  MultipleLevelTreeLayout()
  {
  }

  virtual ~MultipleLevelTreeLayout()
  {
  }

  virtual void setMultipleLevelTreeLayout(TrustTreeNodeList& nodeList);
};

} // namespace chronochat

#endif // CHRONOCHAT_TREE_LAYOUT_HPP
