#ifndef DIGESTTREESCENE_H
#define DIGESTTREESCENE_H

#include <QtGui/QGraphicsScene>
#include "ogdf/basic/GraphAttributes.h"
#include "ogdf/basic/Graph.h"

class DigestTreeScene : public QGraphicsScene
{
  Q_OBJECT

public:
  DigestTreeScene(QWidget *parent = 0);
private:
  ogdf::Graph m_graph;
};
#endif
