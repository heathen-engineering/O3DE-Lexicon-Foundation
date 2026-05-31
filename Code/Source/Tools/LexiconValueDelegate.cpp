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
#include "LexiconTableModel.h"

#include <FoundationLocalisation/LexiconHintType.h>

#include <QLineEdit>
#include <QPlainTextEdit>
#include <QScrollBar>

namespace FoundationLocalisation
{
    static constexpr int kMaxEditLines = 4;

    static bool isStringHint(const QModelIndex& index)
    {
        // Look up the ColType sibling to determine the hint for this row
        const QModelIndex typeIdx =
            index.siblingAtColumn(LexiconTableModel::ColType);
        if (!typeIdx.isValid()) return true; // default to string behaviour
        const auto hint = static_cast<LexiconHintType>(
            typeIdx.data(Qt::EditRole).toInt());
        return (hint == LexiconHintType::None || hint == LexiconHintType::String);
    }

    LexiconValueDelegate::LexiconValueDelegate(QObject* parent)
        : QStyledItemDelegate(parent)
    {
    }

    QWidget* LexiconValueDelegate::createEditor(QWidget* parent,
                                                  const QStyleOptionViewItem& option,
                                                  const QModelIndex& index) const
    {
        if (isStringHint(index))
        {
            auto* editor = new QPlainTextEdit(parent);
            editor->setFrameShape(QFrame::NoFrame);
            editor->setLineWrapMode(QPlainTextEdit::WidgetWidth);
            editor->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            editor->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            editor->document()->setDocumentMargin(2);

            // Commit on Ctrl+Enter; plain Enter inserts a newline
            return editor;
        }
        else
        {
            auto* editor = new QLineEdit(parent);
            editor->setFrame(false);
            return editor;
        }
    }

    void LexiconValueDelegate::setEditorData(QWidget* editor,
                                               const QModelIndex& index) const
    {
        const QString value = index.data(Qt::EditRole).toString();

        if (auto* pte = qobject_cast<QPlainTextEdit*>(editor))
            pte->setPlainText(value);
        else if (auto* le = qobject_cast<QLineEdit*>(editor))
            le->setText(value);
    }

    void LexiconValueDelegate::setModelData(QWidget* editor,
                                              QAbstractItemModel* model,
                                              const QModelIndex& index) const
    {
        if (auto* pte = qobject_cast<QPlainTextEdit*>(editor))
            model->setData(index, pte->toPlainText(), Qt::EditRole);
        else if (auto* le = qobject_cast<QLineEdit*>(editor))
            model->setData(index, le->text(), Qt::EditRole);
    }

    void LexiconValueDelegate::updateEditorGeometry(QWidget* editor,
                                                      const QStyleOptionViewItem& option,
                                                      const QModelIndex& /*index*/) const
    {
        if (auto* pte = qobject_cast<QPlainTextEdit*>(editor))
        {
            const QFontMetrics fm(pte->font());
            const int lineH   = fm.lineSpacing() + 2;
            const int minH    = lineH + 8;                   // 1 line + margins
            const int maxH    = kMaxEditLines * lineH + 8;   // 4 lines + margins

            // Current content height (clamped to [1..4] lines)
            const int docH = static_cast<int>(pte->document()->size().height());
            const int prefH = qBound(minH, docH + 8, maxH);

            // Position below the cell, expanding downward
            const QRect r = option.rect;
            editor->setGeometry(r.x(), r.y(), r.width(), qMax(prefH, r.height()));
        }
        else
        {
            editor->setGeometry(option.rect);
        }
    }

} // namespace FoundationLocalisation

#include <moc_LexiconValueDelegate.cpp>
