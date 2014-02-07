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

#include "contactpanel.h"
#include "contact-storage.h"
#include "dns-storage.h"
#include "contact-manager.h"
#include "logging.h"
#include <ndn-cpp-dev/face.hpp>
#include <boost/thread/thread.hpp>

INIT_LOGGER("MAIN");

using namespace ndn;

class NewApp : public QApplication
{
public:
  NewApp(int & argc, char ** argv)
    : QApplication(argc, argv)
  { }

  bool notify(QObject * receiver, QEvent * event) 
  {
    try {
        return QApplication::notify(receiver, event);
    } 
    catch(std::exception& e){
      std::cerr << "Exception thrown:" << e.what() << std::endl;
      return false;
    }
    
  }
};

void runIO(shared_ptr<boost::asio::io_service> ioService)
{
  try{
    ioService->run();
  }catch(std::runtime_error& e){
    std::cerr << e.what() << std::endl;
  }
}

int main(int argc, char *argv[])
{
  NewApp app(argc, argv);
  
  shared_ptr<Face> face = make_shared<Face>();
  ContactPanel contactPanel(face);

  contactPanel.show ();
  contactPanel.activateWindow ();
  contactPanel.raise ();

  boost::thread (runIO, face->ioService());
  
  return app.exec();
}
