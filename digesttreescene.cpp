#include "digesttreescene.h"
#include <QtGui>
#include "ogdf/basic/Array.h"
#include "ogdf/basic/Graph_d.h"
#include "ogdf/tree/TreeLayout.h"
#include <vector>
#include <iostream>
#include <assert.h>
#include <boost/lexical_cast.hpp>

static const double Pi = 3.14159265358979323846264338327950288419717;

static void 
testDraw(DigestTreeScene * scene) 
{
  std::string prefix[5] = {"/ndn/1", "/ndn/2", "/ndn/3", "/ndn/4", "/ndn/5"};
  std::string nick[5] = {"tom", "jerry", "jason", "michael", "hurry"};
  std::vector<Sync::MissingDataInfo> v;
  for (int i = 0; i < 5; i++)
  {
    Sync::MissingDataInfo mdi = {prefix[i], Sync::SeqNo(0), Sync::SeqNo(i * (2 << i) )};
    v.push_back(mdi);
  }

  scene->processUpdate(v, "12341234@!#%!@");

  for (int i = 0; i < 5; i++)
  {
   scene-> msgReceived(prefix[i].c_str(), nick[i].c_str());
  }
}

DigestTreeScene::DisplayUserPtr DigestTreeScene::DisplayUserNullPtr;

DigestTreeScene::DigestTreeScene(QWidget *parent)
  : QGraphicsScene(parent)
{
  previouslyUpdatedUser = DisplayUserNullPtr;
  testDraw(this);
}

void
DigestTreeScene::processUpdate(std::vector<Sync::MissingDataInfo> &v, QString digest)
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
DigestTreeScene::plot(QString digest)
{
  clear();
  m_graph.clear();
  int n = m_roster.size();
  ogdf::node root = m_graph.newNode();
  int rootIndex = root->index();
  for (int i = 0; i < n; i++) {
     ogdf::node leaf = m_graph.newNode();
     m_graph.newEdge(root, leaf);
  }
  ogdf::GraphAttributes GA(m_graph);

  int nodeSize = 40;
  int siblingDistance = 100, levelDistance = 100;
  ogdf::TreeLayout layout;
  layout.siblingDistance(siblingDistance);
  layout.levelDistance(levelDistance);
  layout.callSortByPositions(GA, m_graph);

  int width = GA.boundingBox().width();
  int height = GA.boundingBox().height();
  //setSceneRect(QRect(- (width + nodeSize) / 2, - 50, width + nodeSize, height + nodeSize));
  GA.setAllWidth(nodeSize);
  GA.setAllHeight(nodeSize);

  plotEdge(GA);
  plotNode(GA, rootIndex, digest);

  previouslyUpdatedUser = DisplayUserNullPtr;

}

void
DigestTreeScene::plotEdge(ogdf::GraphAttributes &GA)
{
  ogdf::edge e;
  forall_edges(e, m_graph) {
    ogdf::node source = e->source();
    ogdf::node target = e->target();
    int nodeSize = GA.width(target);
    int x1 = GA.x(source), y1 = -GA.y(source);
    int x2 = GA.x(target), y2 = -GA.y(target);
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
DigestTreeScene::plotNode(ogdf::GraphAttributes &GA, int rootIndex, QString digest)
{
  ogdf::node n;
  RosterIterator it(m_roster);
  forall_nodes(n, m_graph) {
    double x = GA.x(n);
    double y = -GA.y(n);
    double w = GA.width(n);
    double h = GA.height(n);
    int rim = 3;
    QRectF boundingRect(x, y, w, h);
    QRectF innerBoundingRect(x + rim, y + rim, w - rim * 2, h - rim * 2);

    if (n->index() == rootIndex) 
    {
      addRect(boundingRect, QPen(Qt::black), QBrush(Qt::darkRed));
      addRect(innerBoundingRect, QPen(Qt::black), QBrush(Qt::lightGray));

      QRectF digestRect(x - w, y - h, 3 * w, 30);
      addRect(digestRect, QPen(Qt::darkCyan), QBrush(Qt::darkCyan));
      QGraphicsTextItem *digestItem = addText(digest);
      QRectF digestBoundingRect = digestItem->boundingRect();
      digestItem->setDefaultTextColor(Qt::white);
      digestItem->setFont(QFont("Cursive", 12, QFont::Bold));
      digestItem->setPos(x - w + (3 * w - digestBoundingRect.width()) / 2, y - h + 5);
      m_rootDigest = digestItem;
    }
    else
    {
      if (it.hasNext()) 
      {
        it.next();
      }
      else 
      {
        abort();
      }
      DisplayUserPtr p = it.value();
      QGraphicsRectItem *rectItem = addRect(boundingRect, QPen(Qt::black), QBrush(Qt::darkBlue));
      p->setRimRectItem(rectItem);

      QGraphicsRectItem *innerRectItem = addRect(innerBoundingRect, QPen(Qt::black), QBrush(Qt::lightGray));
      p->setInnerRectItem(innerRectItem);

      std::string s = boost::lexical_cast<std::string>(p->getSeqNo().getSeq());
      QGraphicsTextItem *seqItem = addText(s.c_str());
      seqItem->setFont(QFont("Cursive", 12, QFont::Bold));
      QRectF seqBoundingRect = seqItem->boundingRect(); 
      seqItem->setPos(x + w / 2 - seqBoundingRect.width() / 2, y + h / 2 - seqBoundingRect.height() / 2);
      p->setSeqTextItem(seqItem);

      QRectF textRect(x - w / 2, y + h, 2 * w, 30);
      QGraphicsRectItem *nickRectItem = addRect(textRect, QPen(Qt::darkCyan), QBrush(Qt::darkCyan));
      p->setNickRectItem(nickRectItem);
      QGraphicsTextItem *nickItem = addText(p->getNick());
      QRectF textBoundingRect = nickItem->boundingRect();
      nickItem->setDefaultTextColor(Qt::white);
      nickItem->setFont(QFont("Cursive", 12, QFont::Bold));
      nickItem->setPos(x + w / 2 - textBoundingRect.width() / 2, y + h + 5);
      p->setNickTextItem(nickItem);
    }

  }
}

