/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Zhenkai Zhu <zhenkai@cs.ucla.edu>
 *         Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#ifndef DIGESTTREESCENE_H
#define DIGESTTREESCENE_H

#include "treelayout.h"

#include <QtGui/QGraphicsScene>
#include <QColor>
#include <QMap>

#ifndef Q_MOC_RUN
#include <sync-seq-no.h>
#include <sync-logic.h>
#include <ctime>
#include <vector>
#include <boost/shared_ptr.hpp>
#endif

const int FRESHNESS = 60;

class QGraphicsTextItem;

class User;
class DisplayUser;
typedef boost::shared_ptr<DisplayUser> DisplayUserPtr;
static DisplayUserPtr DisplayUserNullPtr;

class DigestTreeScene : public QGraphicsScene
{
  Q_OBJECT

typedef QMap<QString, DisplayUserPtr> Roster;
typedef QMap<QString, DisplayUserPtr>::iterator Roster_iterator;
typedef QMapIterator<QString, DisplayUserPtr> RosterIterator;

public:
  DigestTreeScene(QWidget *parent = 0);
  void processUpdate(const std::vector<Sync::MissingDataInfo> &v, QString digest);
  void msgReceived(QString prefix, QString nick);
  void clearAll();
  bool removeNode(const QString prefix);
  void plot(QString digest);
  QStringList getRosterList();
  void setCurrentPrefix(QString prefix) {m_currentPrefix = prefix;}
  QMap<QString, DisplayUserPtr> getRosterFull() { return m_roster;}

signals:
  void replot();
  void rosterChanged(QStringList);

private slots:
  void emitReplot();

private:
  void plotEdge(const std::vector<TreeLayout::Coordinate> &v, int nodeSize);
  void plotNode(const std::vector<TreeLayout::Coordinate> &v, QString digest, int nodeSize);
  void reDrawNode(DisplayUserPtr p, QColor rimColor);

private:
  Roster m_roster;
  QGraphicsTextItem *m_rootDigest; 
  DisplayUserPtr previouslyUpdatedUser;
  QString m_currentPrefix;
};

class User 
{
public:
  User():m_received(time(NULL)) {}
  User(QString n, QString p, QString c): m_nick(n), m_prefix(p), m_chatroom(c), m_received(time(NULL)) {}
  void setNick(QString nick) {m_nick = nick;}
  void setPrefix(QString prefix) {m_prefix = prefix;}
  void setChatroom(QString chatroom) {m_chatroom = chatroom;}
  void setSeq(Sync::SeqNo seq) {m_seq = seq;}
  void setReceived(time_t t) {m_received = t;}
  void setOriginPrefix(QString originPrefix) { m_originPrefix = originPrefix;}
  QString getNick() { return m_nick;}
  QString getPrefix() { return m_prefix;}
  QString getChatroom() { return m_chatroom;}
  QString getOriginPrefix() { return m_originPrefix;}
  Sync::SeqNo getSeqNo() { return m_seq;}
  time_t getReceived() { return m_received;}
private:
  QString m_nick;
  QString m_prefix;
  QString m_chatroom;
  QString m_originPrefix;
  Sync::SeqNo m_seq;
  time_t m_received;
};

class DisplayUser : public User 
{
public:
  DisplayUser():m_seqTextItem(NULL), m_nickTextItem(NULL), m_rimRectItem(NULL) {}
  DisplayUser(QString n, QString p , QString c): User(n, p, c), m_seqTextItem(NULL), m_nickTextItem(NULL), m_rimRectItem(NULL) {}
  QGraphicsTextItem *getSeqTextItem() {return m_seqTextItem;}
  QGraphicsTextItem *getNickTextItem() {return m_nickTextItem;}
  QGraphicsRectItem *getRimRectItem() {return m_rimRectItem;}
  QGraphicsRectItem *getInnerRectItem() {return m_innerRectItem;}
  QGraphicsRectItem *getNickRectItem() {return m_nickRectItem;}
  void setSeqTextItem(QGraphicsTextItem *item) { m_seqTextItem = item;}
  void setNickTextItem(QGraphicsTextItem *item) { m_nickTextItem = item;}
  void setRimRectItem(QGraphicsRectItem *item) {m_rimRectItem = item;}
  void setInnerRectItem(QGraphicsRectItem *item) {m_innerRectItem = item;}
  void setNickRectItem(QGraphicsRectItem *item) {m_nickRectItem = item;}
private:
  QGraphicsTextItem *m_seqTextItem;
  QGraphicsTextItem *m_nickTextItem;
  QGraphicsRectItem *m_rimRectItem;
  QGraphicsRectItem *m_innerRectItem;
  QGraphicsRectItem *m_nickRectItem;
};

#endif
