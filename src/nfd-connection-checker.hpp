/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#ifndef CHRONOCHAT_NFD_CONNECTION_CHECKER_HPP
#define CHRONOCHAT_NFD_CONNECTION_CHECKER_HPP

#include <QThread>

#ifndef Q_MOC_RUN
#include "common.hpp"
#include <mutex>
#include <boost/thread.hpp>
#include <ndn-cxx/face.hpp>
#endif

namespace chronochat {

class NfdConnectionChecker : public QThread
{
  Q_OBJECT

public:
  NfdConnectionChecker(QObject* parent = nullptr);

protected:
  void
  run();

signals:
  void
  nfdConnected();

public slots:
  void
  shutdown();

private:
  bool m_nfdConnected;
  std::mutex m_nfdMutex;

  unique_ptr<ndn::Face> m_face;
};

} // namespace chronochat

#endif // CHRONOCHAT_NFD_CONNECTION_CHECKER_HPP
