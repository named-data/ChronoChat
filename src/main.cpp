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
#include <QSystemTrayIcon>

#include "chronochat.h"
#include "contactpanel.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);


//   app.setWindowIcon(QIcon("/Users/yuyingdi/Develop/QT/demo.icns"));
// // #else
// // 	app.setWindowIcon(QIcon("/Users/yuyingdi/Develop/QT/images/icon_large.png"));
// // #endif

  ContactPanel contactPanel;

  contactPanel.show ();
  contactPanel.activateWindow ();
  contactPanel.raise ();
  
  return app.exec();
}
