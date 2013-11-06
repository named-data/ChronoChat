/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef LINKNDN_ENDORSE_SQL_TABLE_MODEL_H
#define LINKNDN_ENDORSE_SQL_TABLE_MODEL_H

#include <QtSql/QSqlTableModel>

class EndorseSqlTableModel : public QSqlTableModel
{
public:
  EndorseSqlTableModel(QObject * parent = 0, QSqlDatabase db = QSqlDatabase());
  
  virtual
  ~EndorseSqlTableModel();

  Qt::ItemFlags 
  flags ( const QModelIndex & index );

  bool 
  setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );

  QVariant 
  data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
};

#endif
