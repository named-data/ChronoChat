#ifndef DIGESTTREEVIEWER_H
#define DIGESTTREEVIEWER_H
#include <QtGui/QGraphicsView>

class DigestTreeViewer : public QGraphicsView
{ 
  Q_OBJECT

public:
  DigestTreeViewer(QWidget *parent = 0);
};

#endif
