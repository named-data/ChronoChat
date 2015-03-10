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
#include <QTextCodec>
// #include <QSystemTrayIcon>

#include "controller.hpp"
#include "logging.h"
#include <ndn-cxx/face.hpp>
#include <boost/thread/thread.hpp>

class NewApp : public QApplication
{
public:
  NewApp(int& argc, char** argv)
    : QApplication(argc, argv)
  {
  }

  bool
  notify(QObject* receiver, QEvent* event)
  {
    try {
        return QApplication::notify(receiver, event);
    }
    catch (std::exception& e) {
      std::cerr << "Exception thrown:" << e.what() << std::endl;
      return false;
    }

  }
};

int main(int argc, char *argv[])
{
  NewApp app(argc, argv);

  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
  QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));

  chronochat::Controller controller;

  app.setQuitOnLastWindowClosed(false);

  return app.exec();
}
