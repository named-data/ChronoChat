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

#include "digest-tree-scene.hpp"

#include <QtGui>

#ifndef Q_MOC_RUN
#include <vector>
#include <iostream>
#include <assert.h>
#include <boost/lexical_cast.hpp>
#include <memory>
#endif

namespace chronochat {

static const double Pi = 3.14159265358979323846264338327950288419717;
static const int NODE_SIZE = 40;

//DisplayUserPtr DisplayUserNullPtr;

DigestTreeScene::DigestTreeScene(QWidget *parent)
  : QGraphicsScene(parent)
{
  m_previouslyUpdatedUser = DisplayUserNullPtr;
}

void
DigestTreeScene::processSyncUpdate(const std::vector<chronochat::NodeInfo>& nodeInfos,
                                   const QString& digest)
{
  m_rootDigest = digest;
}

void
DigestTreeScene::updateNode(QString sessionPrefix, QString nick, uint64_t seqNo)
{
  Roster_iterator it = m_roster.find(sessionPrefix);
  if (it == m_roster.end()) {
    DisplayUserPtr p(new DisplayUser());
    p->setPrefix(sessionPrefix);
    p->setSeq(seqNo);
    m_roster.insert(p->getPrefix(), p);
    plot(m_rootDigest);
  }
  else {
    it.value()->setSeq(seqNo);
    DisplayUserPtr p = it.value();
    QGraphicsTextItem *item = p->getSeqTextItem();
    QGraphicsRectItem *rectItem = p->getInnerRectItem();
    std::string s = boost::lexical_cast<std::string>(p->getSeqNo());
    item->setPlainText(s.c_str());
    QRectF textBR = item->boundingRect();
    QRectF rectBR = rectItem->boundingRect();
    item->setPos(rectBR.x() + (rectBR.width() - textBR.width())/2,
                 rectBR.y() + (rectBR.height() - textBR.height())/2);
  }
  m_displayRootDigest->setPlainText(m_rootDigest);
  updateNick(sessionPrefix, nick);
}

void
DigestTreeScene::updateNick(QString sessionPrefix, QString nick)
{
  Roster_iterator it = m_roster.find(sessionPrefix);
  if (it != m_roster.end()) {
    DisplayUserPtr p = it.value();
    if (nick != p->getNick()) {
      p->setNick(nick);
      QGraphicsTextItem *nickItem = p->getNickTextItem();
      QGraphicsRectItem *nickRectItem = p->getNickRectItem();
      nickItem->setPlainText(p->getNick());
      QRectF rectBR = nickRectItem->boundingRect();
      QRectF nickBR = nickItem->boundingRect();
      nickItem->setPos(rectBR.x() + (rectBR.width() - nickBR.width())/2, rectBR.y() + 5);
    }
  }
}

void
DigestTreeScene::messageReceived(QString sessionPrefix)
{
  Roster_iterator it = m_roster.find(sessionPrefix);
  if (it != m_roster.end()) {
    DisplayUserPtr p = it.value();

    reDrawNode(p, Qt::red);

    if (m_previouslyUpdatedUser != DisplayUserNullPtr && m_previouslyUpdatedUser != p) {
      reDrawNode(m_previouslyUpdatedUser, Qt::darkBlue);
    }

    m_previouslyUpdatedUser = p;
  }
}

void
DigestTreeScene::clearAll()
{
  clear();
  m_roster.clear();
}

void
DigestTreeScene::removeNode(const QString sessionPrefix)
{
  m_roster.remove(sessionPrefix);
  plot(m_rootDigest);
}

QStringList
DigestTreeScene::getRosterList()
{
  QStringList rosterList;
  RosterIterator it(m_roster);
  while (it.hasNext()) {
    it.next();
    DisplayUserPtr p = it.value();
    if (p != DisplayUserNullPtr) {
      rosterList << "- " + p->getNick();
    }
  }
  return rosterList;
}

QStringList
DigestTreeScene::getRosterPrefixList()
{
  QStringList prefixList;
  RosterIterator it(m_roster);
  while (it.hasNext()) {
    it.next();
    DisplayUserPtr p = it.value();
    if (p != DisplayUserNullPtr) {
      prefixList << p->getPrefix();
    }
  }
  return prefixList;
}

void
DigestTreeScene::plot(QString rootDigest)
{
  clear();

  shared_ptr<TreeLayout> layout(new OneLevelTreeLayout());
  layout->setSiblingDistance(100);
  layout->setLevelDistance(100);

  std::vector<TreeLayout::Coordinate> childNodesCo(m_roster.size());
  layout->setOneLevelLayout(childNodesCo);
  plotEdge(childNodesCo, NODE_SIZE);
  plotNode(childNodesCo, rootDigest, NODE_SIZE);

  m_previouslyUpdatedUser = DisplayUserNullPtr;
}

void
DigestTreeScene::plotEdge(const std::vector<TreeLayout::Coordinate> &childNodesCo, int nodeSize)
{
  int n = childNodesCo.size();
  for (int i = 0; i < n; i++) {
    double x1 = 0.0, y1 = 0.0;
    double x2 = childNodesCo[i].x, y2 = childNodesCo[i].y;
    QPointF src(x1 + nodeSize/2, y1 + nodeSize/2);
    QPointF dest(x2 + nodeSize/2, y2 + nodeSize/2);
    QLineF line(src, dest);
    double angle = ::acos(line.dx() / line.length());

    double arrowSize = 10;
    QPointF sourceArrowP0 = src + QPointF((nodeSize/2 + 10) * line.dx() / line.length(),
                                          (nodeSize/2 +10) * line.dy() / line.length());
    QPointF sourceArrowP1 = sourceArrowP0 + QPointF(cos(angle + Pi / 3 - Pi/2) * arrowSize,
                                                    sin(angle + Pi / 3 - Pi/2) * arrowSize);
    QPointF sourceArrowP2 = sourceArrowP0 + QPointF(cos(angle + Pi - Pi / 3 - Pi/2) * arrowSize,
                                                    sin(angle + Pi - Pi / 3 - Pi/2) * arrowSize);

    addLine(QLineF(sourceArrowP0, dest), QPen(Qt::black));
    addPolygon(QPolygonF() << sourceArrowP0<< sourceArrowP1 <<
               sourceArrowP2, QPen(Qt::black), QBrush(Qt::black));
  }
}

void
DigestTreeScene::plotNode(const std::vector<TreeLayout::Coordinate>& childNodesCo,
                          QString digest, int nodeSize)
{
  RosterIterator it(m_roster);
  int n = childNodesCo.size();
  int rim = 3;

  // plot root node
  QRectF rootBoundingRect(0, 0, nodeSize, nodeSize);
  QRectF rootInnerBoundingRect(rim, rim, nodeSize - rim * 2, nodeSize - rim * 2);
  addRect(rootBoundingRect, QPen(Qt::black), QBrush(Qt::darkRed));
  addRect(rootInnerBoundingRect, QPen(Qt::black), QBrush(Qt::lightGray));
  QRectF digestRect(- 5.5 * nodeSize , - nodeSize, 12 * nodeSize, 30);
  addRect(digestRect, QPen(Qt::darkCyan), QBrush(Qt::darkCyan));

  QGraphicsTextItem *digestItem = addText(digest);
  QRectF digestBoundingRect = digestItem->boundingRect();
  digestItem->setDefaultTextColor(Qt::black);
  digestItem->setFont(QFont("Cursive", 12, QFont::Bold));
  digestItem->setPos(- 4.5 * nodeSize + (12 * nodeSize - digestBoundingRect.width()) / 2,
                     - nodeSize + 5);
  m_displayRootDigest = digestItem;

  // plot child nodes
  for (int i = 0; i < n; i++) {
    if (it.hasNext())
      it.next();
    else
      abort();

    double x = childNodesCo[i].x;
    double y = childNodesCo[i].y;
    QRectF boundingRect(x, y, nodeSize, nodeSize);
    QRectF innerBoundingRect(x + rim, y + rim, nodeSize - rim * 2, nodeSize - rim * 2);
    DisplayUserPtr p = it.value();
    QGraphicsRectItem *rectItem = addRect(boundingRect, QPen(Qt::black), QBrush(Qt::darkBlue));
    p->setRimRectItem(rectItem);

    QGraphicsRectItem *innerRectItem = addRect(innerBoundingRect,
                                               QPen(Qt::black),
                                               QBrush(Qt::lightGray));
    p->setInnerRectItem(innerRectItem);

    std::string s = boost::lexical_cast<std::string>(p->getSeqNo());
    QGraphicsTextItem *seqItem = addText(s.c_str());
    seqItem->setFont(QFont("Cursive", 12, QFont::Bold));
    QRectF seqBoundingRect = seqItem->boundingRect();
    seqItem->setPos(x + nodeSize / 2 - seqBoundingRect.width() / 2,
                    y + nodeSize / 2 - seqBoundingRect.height() / 2);
    p->setSeqTextItem(seqItem);

    QRectF textRect(x - nodeSize / 2, y + nodeSize, 2 * nodeSize, 30);
    QGraphicsRectItem *nickRectItem = addRect(textRect, QPen(Qt::darkCyan), QBrush(Qt::darkCyan));
    p->setNickRectItem(nickRectItem);
    QGraphicsTextItem *nickItem = addText(p->getNick());
    QRectF textBoundingRect = nickItem->boundingRect();
    nickItem->setDefaultTextColor(Qt::white);
    nickItem->setFont(QFont("Cursive", 12, QFont::Bold));
    nickItem->setPos(x + nodeSize / 2 - textBoundingRect.width() / 2, y + nodeSize + 5);
    p->setNickTextItem(nickItem);
  }

}

void
DigestTreeScene::reDrawNode(DisplayUserPtr p, QColor rimColor)
{
    QGraphicsRectItem *rimItem = p->getRimRectItem();
    rimItem->setBrush(QBrush(rimColor));
    QGraphicsRectItem *innerItem = p->getInnerRectItem();
    innerItem->setBrush(QBrush(Qt::lightGray));
    QGraphicsTextItem *seqTextItem = p->getSeqTextItem();
    std::string s = boost::lexical_cast<std::string>(p->getSeqNo());
    seqTextItem->setPlainText(s.c_str());
    QRectF textBR = seqTextItem->boundingRect();
    QRectF innerBR = innerItem->boundingRect();
    seqTextItem->setPos(innerBR.x() + (innerBR.width() - textBR.width())/2,
                        innerBR.y() + (innerBR.height() - textBR.height())/2);
}

} // namespace chronochat

#if WAF
#include "digest-tree-scene.moc"
// #include "digest-tree-scene.cpp.moc"
#endif
