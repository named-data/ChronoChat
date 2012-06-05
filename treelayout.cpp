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
