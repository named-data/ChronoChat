#include <QtGui>
#include <vector>
#include <iostream>
#include <assert.h>
#include <boost/lexical_cast.hpp>
#include <memory>
#include "digesttreescene.h"

static const double Pi = 3.14159265358979323846264338327950288419717;


DigestTreeScene::DisplayUserPtr DigestTreeScene::DisplayUserNullPtr;

DigestTreeScene::DigestTreeScene(QWidget *parent)
  : QGraphicsScene(parent)
{
  previouslyUpdatedUser = DisplayUserNullPtr;
}

void
DigestTreeScene::processUpdate(const std::vector<Sync::MissingDataInfo> &v, QString digest)
{
  int n = v.size();
  bool rePlot = false; 
  for (int i = 0; i < n; i++) 
  {
    Roster_iterator it = m_roster.find(v[i].prefix.c_str());
    if (it == m_roster.end()) {
      rePlot = true; 
      DisplayUserPtr p(new DisplayUser());
      p->setPrefix(v[i].prefix.c_str());
      p->setSeq(v[i].high);
      m_roster.insert(p->getPrefix(), p);
    }
  }

  if (rePlot) 
  {
    plot(digest);
  }
  else 
  {
    for (int i = 0; i < n; i++) 
    {
      Roster_iterator it = m_roster.find(v[i].prefix.c_str());
      if (it != m_roster.end()) {
        DisplayUserPtr p = it.value();
        QGraphicsTextItem *item = p->getSeqTextItem();
        QGraphicsRectItem *rectItem = p->getInnerRectItem();
        std::string s = boost::lexical_cast<std::string>(p->getSeqNo().getSeq());
        item->setPlainText(s.c_str());
        QRectF textBR = item->boundingRect();
        QRectF rectBR = rectItem->boundingRect();
        item->setPos(rectBR.x() + (rectBR.width() - textBR.width())/2, rectBR.y() + (rectBR.height() - textBR.height())/2);
      }
    }
    m_rootDigest->setPlainText(digest);
  }
}

void
DigestTreeScene::msgReceived(QString prefix, QString nick)
{
  Roster_iterator it = m_roster.find(prefix);
  if (it != m_roster.end()) 
  {
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

    reDrawNode(p, Qt::red);

    if (previouslyUpdatedUser != DisplayUserNullPtr) 
    {
      reDrawNode(previouslyUpdatedUser, Qt::darkBlue);
    }

    previouslyUpdatedUser = p;
  }
}

void
DigestTreeScene::clearAll()
{
  clear();
  m_roster.clear();
}

void
DigestTreeScene::plot(QString digest)
{
  clear();

  int nodeSize = 40;

  int siblingDistance = 100, levelDistance = 100;
  std::auto_ptr<TreeLayout> layout(new OneLevelTreeLayout());
  layout->setSiblingDistance(siblingDistance);
  layout->setLevelDistance(levelDistance);

  int n = m_roster.size();
  std::vector<TreeLayout::Coordinate> childNodesCo(n);

  layout->setOneLevelLayout(childNodesCo);

  plotEdge(childNodesCo, nodeSize);
  plotNode(childNodesCo, digest, nodeSize);

  previouslyUpdatedUser = DisplayUserNullPtr;

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
    QPointF sourceArrowP0 = src + QPointF((nodeSize/2 + 10) * line.dx() / line.length(),  (nodeSize/2 +10) * line.dy() / line.length());
    QPointF sourceArrowP1 = sourceArrowP0 + QPointF(cos(angle + Pi / 3 - Pi/2) * arrowSize,
                                                    sin(angle + Pi / 3 - Pi/2) * arrowSize);
    QPointF sourceArrowP2 = sourceArrowP0 + QPointF(cos(angle + Pi - Pi / 3 - Pi/2) * arrowSize,
                                                         sin(angle + Pi - Pi / 3 - Pi/2) * arrowSize);

    addLine(QLineF(sourceArrowP0, dest), QPen(Qt::black));
    addPolygon(QPolygonF() << sourceArrowP0<< sourceArrowP1 << sourceArrowP2, QPen(Qt::black), QBrush(Qt::black));
  }
}

void
DigestTreeScene::plotNode(const std::vector<TreeLayout::Coordinate> &childNodesCo, QString digest, int nodeSize)
{
  RosterIterator it(m_roster);
  int n = childNodesCo.size();
  int rim = 3;

  // plot root node
  QRectF rootBoundingRect(0, 0, nodeSize, nodeSize);
  QRectF rootInnerBoundingRect(rim, rim, nodeSize - rim * 2, nodeSize - rim * 2);
  addRect(rootBoundingRect, QPen(Qt::black), QBrush(Qt::darkRed));
  addRect(rootInnerBoundingRect, QPen(Qt::black), QBrush(Qt::lightGray));
  QRectF digestRect(- nodeSize, - nodeSize, 3 * nodeSize, 30);
  addRect(digestRect, QPen(Qt::darkCyan), QBrush(Qt::darkCyan));
  QGraphicsTextItem *digestItem = addText(digest);
  QRectF digestBoundingRect = digestItem->boundingRect();
  digestItem->setDefaultTextColor(Qt::white);
  digestItem->setFont(QFont("Cursive", 12, QFont::Bold));
  digestItem->setPos(- nodeSize + (3 * nodeSize - digestBoundingRect.width()) / 2, - nodeSize + 5);
  m_rootDigest = digestItem;

  // plot child nodes
  for (int i = 0; i < n; i++)
  {
    if (it.hasNext()) 
    {
      it.next();
    }
    else 
    {
      abort();
    }

    double x = childNodesCo[i].x;
    double y = childNodesCo[i].y;
    QRectF boundingRect(x, y, nodeSize, nodeSize);
    QRectF innerBoundingRect(x + rim, y + rim, nodeSize - rim * 2, nodeSize - rim * 2);
    DisplayUserPtr p = it.value();
    QGraphicsRectItem *rectItem = addRect(boundingRect, QPen(Qt::black), QBrush(Qt::darkBlue));
    p->setRimRectItem(rectItem);

    QGraphicsRectItem *innerRectItem = addRect(innerBoundingRect, QPen(Qt::black), QBrush(Qt::lightGray));
    p->setInnerRectItem(innerRectItem);

    std::string s = boost::lexical_cast<std::string>(p->getSeqNo().getSeq());
    QGraphicsTextItem *seqItem = addText(s.c_str());
    seqItem->setFont(QFont("Cursive", 12, QFont::Bold));
    QRectF seqBoundingRect = seqItem->boundingRect(); 
    seqItem->setPos(x + nodeSize / 2 - seqBoundingRect.width() / 2, y + nodeSize / 2 - seqBoundingRect.height() / 2);
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
    std::string s = boost::lexical_cast<std::string>(p->getSeqNo().getSeq());
    seqTextItem->setPlainText(s.c_str());
    QRectF textBR = seqTextItem->boundingRect();
    QRectF innerBR = innerItem->boundingRect();
    seqTextItem->setPos(innerBR.x() + (innerBR.width() - textBR.width())/2, innerBR.y() + (innerBR.height() - textBR.height())/2);
}

