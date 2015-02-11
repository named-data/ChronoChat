/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2013, Regents of the University of California
 *                     Yingdi Yu
 *
 * BSD license, See the LICENSE file for more information
 *
 * Author: Yingdi Yu <yingdi@cs.ucla.edu>
 */

#include "endorse-combobox-delegate.hpp"

#include <QComboBox>
#include <QApplication>

#ifndef Q_MOC_RUN
#include "logging.h"
#endif

namespace chronochat {

EndorseComboBoxDelegate::EndorseComboBoxDelegate(QObject* parent)
 : QItemDelegate(parent)
{
  m_items.push_back("Not Endorsed");
  m_items.push_back("Endorsed");
}


QWidget*
EndorseComboBoxDelegate::createEditor(QWidget* parent,
                                      const QStyleOptionViewItem& /* option */,
                                      const QModelIndex& /* index */) const
{
  QComboBox* editor = new QComboBox(parent);
  for (unsigned int i = 0; i < m_items.size(); ++i)
    editor->addItem(m_items[i].c_str());
  return editor;
}

void
EndorseComboBoxDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
  QComboBox* comboBox = static_cast<QComboBox*>(editor);
  int value = index.model()->data(index, Qt::EditRole).toUInt();
  comboBox->setCurrentIndex(value);
}

void
EndorseComboBoxDelegate::setModelData(QWidget* editor,
                                      QAbstractItemModel* model,
                                      const QModelIndex& index) const
{
  QComboBox* comboBox = static_cast<QComboBox*>(editor);
  model->setData(index, comboBox->currentIndex(), Qt::EditRole);
}

void
EndorseComboBoxDelegate::updateEditorGeometry(QWidget* editor,
                                              const QStyleOptionViewItem& option,
                                              const QModelIndex& /* index */) const
{
  editor->setGeometry(option.rect);
}

void
EndorseComboBoxDelegate::paint(QPainter* painter,
                               const QStyleOptionViewItem& option,
                               const QModelIndex& index) const
{
  QStyleOptionViewItemV4 myOption = option;
  QString text = m_items[index.model()->data(index, Qt::EditRole).toUInt()].c_str();

  myOption.text = text;

  QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &myOption, painter);
}

} // namespace chronochat

#if WAF
#include "endorse-combobox-delegate.moc"
// #include "endorse-combobox-delegate.cpp.moc"
#endif
