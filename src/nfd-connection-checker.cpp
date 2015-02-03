/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Qiuhan Ding <qiuhanding@cs.ucla.edu>
 */

#include "nfd-connection-checker.hpp"

#ifndef Q_MOC_RUN

#endif

namespace chronochat {

static const int CONNECTION_RETRY_TIMER = 2;

NfdConnectionChecker::NfdConnectionChecker(QObject* parent)
  :QThread(parent)
{
}

void
NfdConnectionChecker::run()
{
  m_face = unique_ptr<ndn::Face>(new ndn::Face);
#ifdef BOOST_THREAD_USES_CHRONO
  time::seconds reconnetTimer = time::seconds(CONNECTION_RETRY_TIMER);
#else
  boost::posix_time::time_duration reconnectTimer;
  reconnectTimer = boost::posix_time::seconds(CONNECTION_RETRY_TIMER);
#endif
  do {
    {
      std::lock_guard<std::mutex>lock(m_nfdMutex);
      m_nfdConnected = true;
    }
    try {
      m_face->expressInterest(Interest("/localhost/nfd/status"),
                              [&] (const Interest& interest, const Data& data) {
                                m_face->shutdown();
                              },
                              [] (const Interest& interest) {});
      m_face->processEvents(time::milliseconds::zero(), true);
    }
    catch (std::runtime_error& e) {
      {
        std::lock_guard<std::mutex>lock(m_nfdMutex);
        m_nfdConnected = false;
      }
#ifdef BOOST_THREAD_USES_CHRONO
      boost::this_thread::sleep_for(reconnetTimer);
#else
      boost::this_thread::sleep(reconnectTimer);
#endif
    }
  } while (!m_nfdConnected);
  emit nfdConnected();
}

void
NfdConnectionChecker::shutdown()
{
  // In this case, we just stop checking the nfd connection and exit
  std::lock_guard<std::mutex>lock(m_nfdMutex);
  m_nfdConnected = true;
}

} // namespace chronochat

#if WAF
#include "nfd-connection-checker.moc"
// #include "nfd-connection-checker.cpp.moc"
#endif
