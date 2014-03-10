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

#include "tree-layout.h"

#include <iostream>

void
OneLevelTreeLayout::setOneLevelLayout(std::vector<Coordinate> &childNodesCo)
{
  if (childNodesCo.empty())
  {
    return;
  }
  double y = getLevelDistance();
  double sd = getSiblingDistance();
  int n = childNodesCo.size();
  double x = - (n - 1) * sd / 2;
  for (int i = 0; i < n; i++)
  {
    childNodesCo[i].x = x;
    childNodesCo[i].y = y;
    x += sd;
  }
}

void
MultipleLevelTreeLayout::setMultipleLevelTreeLayout(TrustTreeNodeList& nodeList)
{
  if(nodeList.empty())
    return;

  double ld = getLevelDistance();
  double sd = getSiblingDistance();

  std::map<int, double> layerSpan;

  TrustTreeNodeList::iterator it  = nodeList.begin();
  TrustTreeNodeList::iterator end = nodeList.end();
  for(; it != end; it++)
    {
      int layer = (*it)->level();
      (*it)->y = layer * ld;
      (*it)->x = layerSpan[layer];
      layerSpan[layer] += sd;
    }

  std::map<int, double>::iterator layerIt  = layerSpan.begin();
  std::map<int, double>::iterator layerEnd = layerSpan.end();
  for(; layerIt != layerEnd; layerIt++)
    {
      double shift = (layerIt->second - sd) / 2;
      layerIt->second = shift;
    }

  it  = nodeList.begin();
  end = nodeList.end();
  for(; it != end; it++)
    {
      (*it)->x -= layerSpan[(*it)->level()];
    }
}
