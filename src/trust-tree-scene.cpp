/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "trust-tree-scene.hpp"

#include <QtGui>

#ifndef Q_MOC_RUN
#include <assert.h>
#include <memory>
#endif

namespace chronochat {

static const double Pi = 3.14159265358979323846264338327950288419717;

TrustTreeScene::TrustTreeScene(QWidget* parent)
  : QGraphicsScene(parent)
{
}

void
TrustTreeScene::plotTrustTree(TrustTreeNodeList& nodeList)
{
  clear();

  int nodeSize = 40;
  int siblingDistance = 100;
  int levelDistance = 100;

  shared_ptr<MultipleLevelTreeLayout> layout(new MultipleLevelTreeLayout());
  layout->setSiblingDistance(siblingDistance);
  layout->setLevelDistance(levelDistance);
  layout->setMultipleLevelTreeLayout(nodeList);

  plotEdge(nodeList, nodeSize);
  plotNode(nodeList, nodeSize);
}

void
TrustTreeScene::plotEdge(const TrustTreeNodeList& nodeList, int nodeSize)
{
  for (TrustTreeNodeList::const_iterator it = nodeList.begin(); it != nodeList.end(); it++) {
    TrustTreeNodeList& introducees = (*it)->getIntroducees();
    for (TrustTreeNodeList::iterator eeIt = introducees.begin();
         eeIt != introducees.end(); eeIt++) {
      if ((*it)->level() >= (*eeIt)->level())
        continue;

      double x1 = (*it)->x;
      double y1 = (*it)->y;
      double x2 = (*eeIt)->x;
      double y2 = (*eeIt)->y;

      QPointF src(x1 + nodeSize/2, y1 + nodeSize/2);
      QPointF dest(x2 + nodeSize/2, y2 + nodeSize/2);
      QLineF line(src, dest);
      double angle = ::acos(line.dx() / line.length());

      double arrowSize = 10;
      QPointF endP0 = src + QPointF((nodeSize/2) * line.dx() / line.dy(), nodeSize/2);
      QPointF sourceArrowP0 = dest + QPointF((-nodeSize/2) * line.dx() / line.dy(),
                                             -nodeSize/2);
      QPointF sourceArrowP1 = sourceArrowP0 + QPointF(-cos(angle - Pi / 6) * arrowSize,
                                                      -sin(angle - Pi / 6) * arrowSize);
      QPointF sourceArrowP2 = sourceArrowP0 + QPointF(-cos(angle + Pi / 6) * arrowSize,
                                                      -sin(angle + Pi / 6) * arrowSize);

      addLine(QLineF(sourceArrowP0, endP0), QPen(Qt::black));
      addPolygon(QPolygonF() << sourceArrowP0 << sourceArrowP1 << sourceArrowP2,
                 QPen(Qt::black), QBrush(Qt::black));
    }
  }
}

void
TrustTreeScene::plotNode(const TrustTreeNodeList& nodeList, int nodeSize)
{
  int rim = 3;

  // plot nodes
  for (TrustTreeNodeList::const_iterator it = nodeList.begin(); it != nodeList.end(); it++) {
    double x = (*it)->x;
    double y = (*it)->y;
    QRectF boundingRect(x, y, nodeSize, nodeSize);
    QRectF innerBoundingRect(x + rim, y + rim, nodeSize - rim * 2, nodeSize - rim * 2);
    addRect(boundingRect, QPen(Qt::black), QBrush(Qt::darkBlue));
    addRect(innerBoundingRect, QPen(Qt::black), QBrush(Qt::lightGray));

    QRectF textRect(x - nodeSize / 2, y + nodeSize, 2 * nodeSize, 30);
    addRect(textRect, QPen(Qt::darkCyan), QBrush(Qt::darkCyan));
    QGraphicsTextItem *nickItem = addText(QString::fromStdString((*it)->name().toUri()));
    nickItem->setDefaultTextColor(Qt::white);
    nickItem->setFont(QFont("Cursive", 8, QFont::Bold));
    nickItem->setPos(x - nodeSize / 2 + 10, y + nodeSize + 5);
  }
}

} //namespace chronochat

#if WAF
#include "trust-tree-scene.moc"
// #include "trust-tree-scene.cpp.moc"
#endif
