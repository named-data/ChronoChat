/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef TRUST_TREE_SCENE_H
#define TRUST_TREE_SCENE_H

#include <QtGui/QGraphicsScene>
#include <QColor>
#include <QMap>

#ifndef Q_MOC_RUN
#include <vector>
#include "trust-tree-node.h"
#include "tree-layout.h"
#endif

class QGraphicsTextItem;

class TrustTreeScene : public QGraphicsScene
{
  Q_OBJECT

public:
  TrustTreeScene(QWidget *parent = 0);

  void plotTrustTree(TrustTreeNodeList& nodeList);

private:
  void plotEdge(const TrustTreeNodeList& nodeList, int nodeSize);
  void plotNode(const TrustTreeNodeList& nodeList, int nodeSize);
};

#endif // TRUST_TREE_SCENE_H
