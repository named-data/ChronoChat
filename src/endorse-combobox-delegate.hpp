/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#ifndef CHRONOS_ENDORSE_COMBOBOX_DELEGATE_HPP
#define CHRONOS_ENDORSE_COMBOBOX_DELEGATE_HPP

#include <QItemDelegate>
#include <string>
#include <vector>

namespace chronos {

class EndorseComboBoxDelegate : public QItemDelegate
{
  Q_OBJECT
public:
  EndorseComboBoxDelegate(QObject* parent = 0);

  // virtual
  // ~ComboBoxDelegate() {}

  QWidget*
  createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;

  void
  setEditorData(QWidget* editor, const QModelIndex& index) const;

  void
  setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;

  void
  updateEditorGeometry(QWidget* editor,
                       const QStyleOptionViewItem& option,
                       const QModelIndex& index) const;

  void
  paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;

private:
  std::vector<std::string> m_items;

};

} // namespace chronos

#endif // CHRONOS_ENDORSE_COMBOBOX_DELEGATE_HPP
