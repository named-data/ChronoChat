#ifndef TREELAYOUT_H
#define TREELAYOUT_H
#include <vector>

class TreeLayout 
{
public:
  struct Coordinate
  {
    double x;
    double y;
  };
  TreeLayout(){}
  virtual void setOneLevelLayout(std::vector<Coordinate> &childNodesCo){};
  void setSiblingDistance(int d) {m_siblingDistance = d;}
  void setLevelDistance(int d) {m_levelDistance = d;}
  int getSiblingDistance() {return m_siblingDistance;}
  int getLevelDistance() {return m_levelDistance;}
  virtual ~TreeLayout(){}
private:
  int m_siblingDistance;
  int m_levelDistance;
};

class OneLevelTreeLayout: public TreeLayout
{
public:
  OneLevelTreeLayout(){}
  virtual void setOneLevelLayout(std::vector<Coordinate> &childNodesCo);
  virtual ~OneLevelTreeLayout(){}
};
#endif
