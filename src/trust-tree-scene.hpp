/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_TRUST_TREE_SCENE_HPP
#define CHRONOCHAT_TRUST_TREE_SCENE_HPP

#include <QtGui/QGraphicsScene>
#include <QColor>
#include <QMap>

#ifndef Q_MOC_RUN
#include "trust-tree-node.hpp"
#include "tree-layout.hpp"
#endif

class QGraphicsTextItem;

namespace chronochat {

class TrustTreeScene : public QGraphicsScene
{
  Q_OBJECT

public:
  TrustTreeScene(QWidget* parent = 0);

  void
  plotTrustTree(chronochat::TrustTreeNodeList& nodeList);

private:
  void
  plotEdge(const chronochat::TrustTreeNodeList& nodeList, int nodeSize);

  void
  plotNode(const chronochat::TrustTreeNodeList& nodeList, int nodeSize);
};

} // namespace chronochat

#endif // CHRONOCHAT_TRUST_TREE_SCENE_HPP
