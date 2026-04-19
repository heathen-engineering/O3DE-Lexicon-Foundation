/*
 * Copyright (c) 2026 Heathen Engineering Limited
 * Irish Registered Company #556277
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "LexiconValueDelegate.h"

#include <QLineEdit>

namespace FoundationLocalisation
{
    LexiconValueDelegate::LexiconValueDelegate(QObject* parent)
        : QStyledItemDelegate(parent)
    {
    }

    QWidget* LexiconValueDelegate::createEditor(QWidget* parent,
                                                 const QStyleOptionViewItem& /*option*/,
                                                 const QModelIndex& /*index*/) const
    {
        auto* editor = new QLineEdit(parent);
        editor->setFrame(false);
        return editor;
    }

    void LexiconValueDelegate::setEditorData(QWidget* editor,
                                              const QModelIndex& index) const
    {
        const QString value = index.data(Qt::EditRole).toString();
        static_cast<QLineEdit*>(editor)->setText(value);
    }

    void LexiconValueDelegate::setModelData(QWidget* editor,
                                             QAbstractItemModel* model,
                                             const QModelIndex& index) const
    {
        auto* lineEdit = static_cast<QLineEdit*>(editor);
        model->setData(index, lineEdit->text(), Qt::EditRole);
    }

    void LexiconValueDelegate::updateEditorGeometry(QWidget* editor,
                                                     const QStyleOptionViewItem& option,
                                                     const QModelIndex& /*index*/) const
    {
        editor->setGeometry(option.rect);
    }

} // namespace FoundationLocalisation

#include <moc_LexiconValueDelegate.cpp>
