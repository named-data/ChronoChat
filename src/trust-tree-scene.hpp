/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOS_TRUST_TREE_SCENE_HPP
#define CHRONOS_TRUST_TREE_SCENE_HPP

#include <QtGui/QGraphicsScene>
#include <QColor>
#include <QMap>

#ifndef Q_MOC_RUN
#include "trust-tree-node.hpp"
#include "tree-layout.hpp"
#endif

class QGraphicsTextItem;

namespace chronos {

class TrustTreeScene : public QGraphicsScene
{
  Q_OBJECT

public:
  TrustTreeScene(QWidget* parent = 0);

  void
  plotTrustTree(chronos::TrustTreeNodeList& nodeList);

private:
  void
  plotEdge(const chronos::TrustTreeNodeList& nodeList, int nodeSize);

  void
  plotNode(const chronos::TrustTreeNodeList& nodeList, int nodeSize);
};

} // namespace chronos

#endif // CHRONOS_TRUST_TREE_SCENE_HPP
