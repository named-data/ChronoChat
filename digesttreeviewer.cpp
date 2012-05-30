#include "digesttreeviewer.h"
#include "digesttreescene.h"
#include <QtGui>

DigestTreeViewer::DigestTreeViewer(QWidget *parent)
  : QGraphicsView(parent) 
{
  DigestTreeScene *scene = new DigestTreeScene(this);
  setScene(scene);
}
