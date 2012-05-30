#include "digesttreescene.h"
#include <QtGui>
#include "ogdf/basic/Array.h"
#include "ogdf/basic/Graph_d.h"
#include "ogdf/tree/TreeLayout.h"
#include <vector>
#include <iostream>
#include <assert.h>

static const double Pi = 3.14159265358979323846264338327950288419717;
static double TwoPi = 2.0 * Pi;

DigestTreeScene::DigestTreeScene(QWidget *parent)
  : QGraphicsScene(parent)
{
  m_graph.clear();
  std::vector<ogdf::node> v(5);
  for (int i = 0; i < 5; i++)
    v[i] = m_graph.newNode();

  m_graph.newEdge(v[0], v[1]);
  m_graph.newEdge(v[0], v[2]);
  m_graph.newEdge(v[0], v[3]);
  m_graph.newEdge(v[0], v[4]);

  ogdf::GraphAttributes GA(m_graph);
  GA.initAttributes(ogdf::GraphAttributes::nodeLabel);
  int nodeWidth = 30, nodeHeight = 30, siblingDistance = 60;
  ogdf::TreeLayout layout;
  layout.siblingDistance(siblingDistance);
  layout.rootSelection(ogdf::TreeLayout::rootIsSource);
  //layout.call(GA);
  layout.callSortByPositions(GA, m_graph);

  int width = GA.boundingBox().width();
  std::cout << "GA width " << width << std::endl;
  int height = GA.boundingBox().height();
  std::cout << "GA height " << height << std::endl;
  setSceneRect(QRect(0, 0, width + nodeWidth, height + nodeHeight));
  GA.setAllWidth(nodeWidth);
  GA.setAllHeight(nodeHeight);

  ogdf::edge e;
  forall_edges(e, m_graph) {
    ogdf::node source = e->source();
    ogdf::node target = e->target();
    int x1 = GA.x(source), y1 = -GA.y(source);
    int x2 = GA.x(target), y2 = -GA.y(target);
  //  QPainterPath p;
    QPointF src(x1 + nodeWidth/2, y1 + nodeHeight/2);
    QPointF dest(x2 + nodeWidth/2, y2 + nodeHeight/2);
    QLineF line(src, dest);
    //p.moveTo(x1 + nodeWidth/2, y1 + nodeHeight/2);
    //p.lineTo(x2 + nodeWidth/2, y2 + nodeHeight/2);
    //addPath(p, QPen(Qt::black), QBrush(Qt::black));
    addLine(line, QPen(Qt::black));
    
    double angle = ::acos(line.dx() / line.length());

    double arrowSize = 10;

   QPointF sourceArrowP0 = src + QPointF(nodeWidth/2 * line.dx() / line.length(),  nodeHeight/2 * line.dy() / line.length());
   //QPointF sourceArrowP0 = src + QPointF(cos(angle) * nodeHeight/2, sin(angle) * nodeHeight/2);

   std::cout << "src " << src.x() << ", " << src.y() << std::endl;
   std::cout << "dest " << dest.x() << ", " << dest.y() << std::endl;
   std::cout << "line dx " << line.dx() << ", dy " << line.dy() << ", lenght << " << line.length() << std::endl;
   std::cout << "sap " << sourceArrowP0.x() << ", " << sourceArrowP0.y() << std::endl;

   QPointF sourceArrowP1 = sourceArrowP0 + QPointF(cos(angle + Pi / 3 - Pi/2) * arrowSize,
                                                    sin(angle + Pi / 3 - Pi/2) * arrowSize);
      QPointF sourceArrowP2 = sourceArrowP0 + QPointF(cos(angle + Pi - Pi / 3 - Pi/2) * arrowSize,
                                                         sin(angle + Pi - Pi / 3 - Pi/2) * arrowSize);
    QLineF line1(sourceArrowP1, sourceArrowP2);
    addLine(line1, QPen(Qt::black));
     
      addPolygon(QPolygonF() << sourceArrowP0<< sourceArrowP1 << sourceArrowP2, QPen(Qt::black), QBrush(Qt::black));
  }

  /*
  ogdf::node n;
  forall_nodes(n, m_graph) {
    double x = GA.x(n);

    double y = -GA.y(n);
    double w = GA.width(n);
    double h = GA.height(n);
    QRectF boundingRect(x, y, w, h);
 
    addEllipse(boundingRect, QPen(Qt::black), QBrush(Qt::green));
    QGraphicsTextItem *text = addText(QString(GA.labelNode(n).cstr()));
    text->setPos(x, y);
 
  }
    */
}

