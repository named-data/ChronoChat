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
#include "contact-storage.h"
#include "dns-storage.h"
#include "contact-manager.h"
#include <ndn.cxx/security/identity/identity-manager.h>
#include <ndn.cxx/security/identity/osx-privatekey-storage.h>
#include <ndn.cxx/security/identity/basic-identity-storage.h>

using namespace ndn;

class NewApp : public QApplication
{
public:
  NewApp(int & argc, char ** argv)
    : QApplication(argc, argv)
  {}

  bool notify(QObject * receiver, QEvent * event) 
  {
    try 
      {
        return QApplication::notify(receiver, event);
      } 
    catch(std::exception& e) 
      {
        std::cerr << "Exception thrown:" << e.what() << endl;
        return false;
      }
    
  }
};

int main(int argc, char *argv[])
{
  NewApp app(argc, argv);


//   app.setWindowIcon(QIcon("/Users/yuyingdi/Develop/QT/demo.icns"));
// // #else
// // 	app.setWindowIcon(QIcon("/Users/yuyingdi/Develop/QT/images/icon_large.png"));
// // #endif

  Ptr<ContactStorage> contactStorage = Ptr<ContactStorage>(new ContactStorage());
  Ptr<DnsStorage> dnsStorage = Ptr<DnsStorage>(new DnsStorage());
  Ptr<ContactManager> contactManager = Ptr<ContactManager>(new ContactManager(contactStorage, dnsStorage));

  ContactPanel contactPanel(contactManager);

  contactPanel.show ();
  contactPanel.activateWindow ();
  contactPanel.raise ();
  
  return app.exec();
}
