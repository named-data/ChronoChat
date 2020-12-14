/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2020, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "controller.hpp"

#include <QApplication>

#include <boost/exception/diagnostic_information.hpp>
#include <iostream>

class ChronoChatApp : public QApplication
{
public:
  using QApplication::QApplication;

  bool
  notify(QObject* receiver, QEvent* event) final
  {
    try {
      return QApplication::notify(receiver, event);
    }
    catch (const std::exception& e) {
      std::cerr << boost::diagnostic_information(e) << std::endl;
      return false;
    }
  }
};

int
main(int argc, char *argv[])
{
  ChronoChatApp app(argc, argv);
  chronochat::Controller controller;
  app.setQuitOnLastWindowClosed(false);

  return app.exec();
}
