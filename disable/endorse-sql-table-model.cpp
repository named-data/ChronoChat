/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "endorse-sql-table-model.h"

#ifndef Q_MOC_RUN
#include "logging.h"
#endif


INIT_LOGGER("EndorseSqlTableModel");

EndorseSqlTableModel::EndorseSqlTableModel(QObject * parent, QSqlDatabase db)
  : QSqlTableModel(parent, db)
{}
  
EndorseSqlTableModel::~EndorseSqlTableModel()
{}

Qt::ItemFlags 
EndorseSqlTableModel::flags ( const QModelIndex & index )
{
  if(index.column() == 3)
    {
      _LOG_DEBUG("index: row: " << index.row() << " " << index.column() << " check!");
      return Qt::ItemIsSelectable|Qt::ItemIsEnabled|Qt::ItemIsUserCheckable;
    }
  else
    {
      _LOG_DEBUG("index: row: " << index.row() << " " << index.column() << " no-check!");
      return  QSqlTableModel::flags(index);
    }
}

bool 
EndorseSqlTableModel::setData ( const QModelIndex & index, const QVariant & value, int role)
{
  bool success = true;
 
  if (index.column() == 3)
    {
      _LOG_DEBUG("setData: " << role);
      if(role == Qt::CheckStateRole)
	{
	  QString val = (value==Qt::Checked)?"1":"0";
	  return QSqlTableModel::setData(index, val, role);
	}
      
      return true;
    }
  else
    {
      return QSqlTableModel::setData(index, value, role);
    }
}

QVariant 
EndorseSqlTableModel::data ( const QModelIndex & index, int role) const
{
  QVariant value = QSqlTableModel::data(index, role);
  // if (index.column() == 3 && role == Qt::CheckStateRole)
  if (index.column() == 3)
    {
      if(role == Qt::CheckStateRole)
	{
	  bool aBool = ((value.toInt() != 0) ? 1 : 0);
	  if (aBool)
	    return Qt::Checked;
	  else
	    return Qt::Unchecked;
	}
      if(role == Qt::DisplayRole)
	  return QVariant();

      return value;
    }
  else
    {
      return value;
    }
}
