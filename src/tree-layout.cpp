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

#include "tree-layout.hpp"
#include <iostream>

namespace chronochat {

using std::vector;
using std::map;

void
OneLevelTreeLayout::setOneLevelLayout(vector<Coordinate>& childNodesCo)
{
  if (childNodesCo.empty())
    return;

  double y = getLevelDistance();
  double sd = getSiblingDistance();
  int n = childNodesCo.size();
  double x = - (n - 1) * sd / 2;
  for (int i = 0; i < n; i++) {
    childNodesCo[i].x = x;
    childNodesCo[i].y = y;
    x += sd;
  }
}

void
MultipleLevelTreeLayout::setMultipleLevelTreeLayout(TrustTreeNodeList& nodeList)
{
  if (nodeList.empty())
    return;

  double ld = getLevelDistance();
  double sd = getSiblingDistance();

  map<int, double> layerSpan;

  for (TrustTreeNodeList::iterator it = nodeList.begin(); it != nodeList.end(); it++) {
    int layer = (*it)->level();
    (*it)->y = layer * ld;
    (*it)->x = layerSpan[layer];
    layerSpan[layer] += sd;
  }

  for (map<int, double>::iterator layerIt = layerSpan.begin();
       layerIt != layerSpan.end(); layerIt++) {
    double shift = (layerIt->second - sd) / 2;
    layerIt->second = shift;
  }

  for (TrustTreeNodeList::iterator it = nodeList.begin(); it != nodeList.end(); it++) {
    (*it)->x -= layerSpan[(*it)->level()];
  }
}

} // namespace chronochat
