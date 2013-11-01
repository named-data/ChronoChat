/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Zhenkai Zhu <zhenkai@cs.ucla.edu>
 *         Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include "treelayout.h"

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
