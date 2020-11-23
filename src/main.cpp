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

#include "controller.hpp"
#include <iostream>
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

  chronochat::Controller controller;

  app.setQuitOnLastWindowClosed(false);

  return app.exec();
}
