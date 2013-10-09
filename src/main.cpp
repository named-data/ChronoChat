/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include <QApplication>

#include "chronochat.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

// #ifdef __APPLE__
// 	app.setWindowIcon(QIcon(":/demo.icns"));
// #else
// 	app.setWindowIcon(QIcon(":/images/icon_large.png"));
// #endif

  ChronoChat dialog;

  dialog.show ();
  
  return app.exec();
}
