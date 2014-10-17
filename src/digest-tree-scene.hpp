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

#ifndef CHRONOS_DIGEST_TREE_SCENE_HPP
#define CHRONOS_DIGEST_TREE_SCENE_HPP

#include <QtGui/QGraphicsScene>
#include <QColor>
#include <QMap>

#ifndef Q_MOC_RUN
#include "tree-layout.hpp"
#include "chat-dialog-backend.hpp"
#include <Leaf.hpp>
#include <ctime>
#include <vector>
#include <boost/shared_ptr.hpp>
#endif

const int FRESHNESS = 60;

class QGraphicsTextItem;

namespace chronos {

class User;
class DisplayUser;
typedef boost::shared_ptr<DisplayUser> DisplayUserPtr;
typedef QMap<QString, DisplayUserPtr> Roster;
typedef QMap<QString, DisplayUserPtr>::iterator Roster_iterator;
typedef QMapIterator<QString, DisplayUserPtr> RosterIterator;

static DisplayUserPtr DisplayUserNullPtr;


class DigestTreeScene : public QGraphicsScene
{
  Q_OBJECT

public:
  DigestTreeScene(QWidget *parent = 0);

  void
  processSyncUpdate(const std::vector<chronos::NodeInfo>& nodeInfos,
                    const QString& digest);

  void
  updateNick(QString sessionPrefix, QString nick);

  void
  messageReceived(QString sessionPrefix);

  void
  clearAll();

  void
  removeNode(const QString sessionPrefix);

  QStringList
  getRosterList();

  QStringList
  getRosterPrefixList();

  void
  plot(QString rootDigest);

private:
  void
  plotEdge(const std::vector<chronos::TreeLayout::Coordinate>& v, int nodeSize);

  void
  plotNode(const std::vector<chronos::TreeLayout::Coordinate>& v, QString digest, int nodeSize);

  void
  reDrawNode(DisplayUserPtr p, QColor rimColor);

private:
  Roster m_roster;

  QString m_rootDigest;
  QGraphicsTextItem* m_displayRootDigest;

  DisplayUserPtr m_previouslyUpdatedUser;
};

class User
{
public:
  User()
  {
  }

  User(QString n, QString p)
    : m_nick(n)
    , m_prefix(p)
  {
  }

  void
  setNick(QString nick)
  {
    m_nick = nick;
  }

  void
  setPrefix(QString prefix)
  {
    m_prefix = prefix;
  }

  void
  setSeq(chronosync::SeqNo seq)
  {
    m_seq = seq;
  }

  QString
  getNick()
  {
    return m_nick;
  }

  QString getPrefix()
  {
    return m_prefix;
  }

  chronosync::SeqNo
  getSeqNo()
  {
    return m_seq;
  }

private:
  QString m_nick;
  QString m_prefix;
  chronosync::SeqNo m_seq;
};

class DisplayUser : public User
{
public:
  DisplayUser()
    : m_seqTextItem(NULL)
    , m_nickTextItem(NULL)
    , m_rimRectItem(NULL)
  {
  }

  DisplayUser(QString n, QString p)
    : User(n, p)
    , m_seqTextItem(NULL)
    , m_nickTextItem(NULL)
    , m_rimRectItem(NULL)
  {
  }

  QGraphicsTextItem*
  getSeqTextItem()
  {
    return m_seqTextItem;
  }

  QGraphicsTextItem*
  getNickTextItem()
  {
    return m_nickTextItem;
  }

  QGraphicsRectItem*
  getRimRectItem()
  {
    return m_rimRectItem;
  }

  QGraphicsRectItem*
  getInnerRectItem()
  {
    return m_innerRectItem;
  }

  QGraphicsRectItem*
  getNickRectItem()
  {
    return m_nickRectItem;
  }

  void
  setSeqTextItem(QGraphicsTextItem* item)
  {
    m_seqTextItem = item;
  }

  void
  setNickTextItem(QGraphicsTextItem* item)
  {
    m_nickTextItem = item;
  }

  void
  setRimRectItem(QGraphicsRectItem* item)
  {
    m_rimRectItem = item;
  }

  void
  setInnerRectItem(QGraphicsRectItem* item)
  {
    m_innerRectItem = item;
  }

  void
  setNickRectItem(QGraphicsRectItem* item)
  {
    m_nickRectItem = item;
  }

private:
  QGraphicsTextItem* m_seqTextItem;
  QGraphicsTextItem* m_nickTextItem;
  QGraphicsRectItem* m_rimRectItem;
  QGraphicsRectItem* m_innerRectItem;
  QGraphicsRectItem* m_nickRectItem;
};

} // namespace chronos

#endif // CHRONOS_DIGEST_TREE_SCENE_HPP
