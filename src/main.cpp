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

int main(int argc, char *argv[])
{
  NewApp app(argc, argv);

  ContactPanel contactPanel;

  contactPanel.show ();
  contactPanel.activateWindow ();
  contactPanel.raise ();
  
  return app.exec();
}
