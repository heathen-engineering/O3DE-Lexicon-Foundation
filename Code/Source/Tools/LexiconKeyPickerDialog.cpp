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

#include "LexiconKeyPickerDialog.h"
#include "LexiconEditorRequestBus.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

#include <AzCore/std/containers/unordered_map.h>

namespace FoundationLocalisation
{
    LexiconKeyPickerDialog::LexiconKeyPickerDialog(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle("Select Localisation Key");
        setMinimumSize(380, 480);
        resize(420, 520);

        auto* layout = new QVBoxLayout(this);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(6);

        // ---- Filter ----
        auto* filterLabel = new QLabel("Filter:", this);
        m_filterEdit = new QLineEdit(this);
        m_filterEdit->setPlaceholderText("Type to filter keys…");
        m_filterEdit->setClearButtonEnabled(true);

        auto* filterRow = new QHBoxLayout();
        filterRow->addWidget(filterLabel);
        filterRow->addWidget(m_filterEdit, 1);
        layout->addLayout(filterRow);

        // ---- Tree ----
        m_tree = new QTreeWidget(this);
        m_tree->setHeaderHidden(true);
        m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tree->setUniformRowHeights(true);
        m_tree->setAnimated(false);
        layout->addWidget(m_tree, 1);

        // ---- Buttons ----
        auto* buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        m_okButton = buttons->button(QDialogButtonBox::Ok);
        m_okButton->setEnabled(false);

        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttons);

        // ---- Connections ----
        connect(m_filterEdit, &QLineEdit::textChanged,
                this, &LexiconKeyPickerDialog::OnFilterChanged);
        connect(m_tree, &QTreeWidget::currentItemChanged,
                this, &LexiconKeyPickerDialog::OnCurrentItemChanged);
        connect(m_tree, &QTreeWidget::itemDoubleClicked,
                this, &LexiconKeyPickerDialog::OnItemDoubleClicked);

        // ---- Populate from the editor key tree ----
        // BroadcastResult requires the result variable type to match the function's
        // return type. GetKnownKeys() returns by const-ref, so we receive a copy.
        AZStd::vector<AZStd::string> keys;
        LexiconEditorRequestBus::BroadcastResult(
            keys, &LexiconEditorRequests::GetKnownKeys);

        if (!keys.empty())
        {
            PopulateTree(keys);
        }
        else
        {
            // No keys yet — show a placeholder so the dialog is still usable
            auto* placeholder = new QTreeWidgetItem(m_tree,
                QStringList("(no .helex keys found)"));
            placeholder->setFlags(Qt::NoItemFlags);
        }
    }

    void LexiconKeyPickerDialog::SetCurrentKey(const AZStd::string& key)
    {
        if (key.empty())
            return;

        const QString qKey = QString::fromUtf8(key.c_str());
        QTreeWidgetItemIterator it(m_tree);
        while (*it)
        {
            if ((*it)->data(0, Qt::UserRole).toString() == qKey)
            {
                m_tree->setCurrentItem(*it);
                m_tree->scrollToItem(*it, QAbstractItemView::PositionAtCenter);
                break;
            }
            ++it;
        }
    }

    AZStd::string LexiconKeyPickerDialog::GetSelectedKey() const
    {
        const QTreeWidgetItem* item = m_tree->currentItem();
        if (!item)
            return {};

        const QString path = item->data(0, Qt::UserRole).toString();
        return AZStd::string(path.toUtf8().constData());
    }

    // ---- Private slots ----

    void LexiconKeyPickerDialog::OnFilterChanged(const QString& text)
    {
        if (text.isEmpty())
        {
            // Reset: show everything, collapse to depth 0
            QTreeWidgetItemIterator it(m_tree);
            while (*it)
            {
                (*it)->setHidden(false);
                ++it;
            }
            m_tree->collapseAll();
            m_tree->expandToDepth(0);
            return;
        }

        for (int i = 0; i < m_tree->topLevelItemCount(); ++i)
        {
            UpdateItemVisibility(m_tree->topLevelItem(i), text);
        }
    }

    void LexiconKeyPickerDialog::OnCurrentItemChanged(
        QTreeWidgetItem* current, [[maybe_unused]] QTreeWidgetItem* previous)
    {
        const bool hasKey = current
            && !current->data(0, Qt::UserRole).toString().isEmpty();
        m_okButton->setEnabled(hasKey);
    }

    void LexiconKeyPickerDialog::OnItemDoubleClicked(
        QTreeWidgetItem* item, [[maybe_unused]] int column)
    {
        if (item && !item->data(0, Qt::UserRole).toString().isEmpty())
        {
            accept();
        }
    }

    // ---- Private helpers ----

    void LexiconKeyPickerDialog::PopulateTree(const AZStd::vector<AZStd::string>& keys)
    {
        m_tree->clear();

        // path segment → owning QTreeWidgetItem
        AZStd::unordered_map<AZStd::string, QTreeWidgetItem*> nodeMap;

        for (const AZStd::string& key : keys)
        {
            const QStringList parts =
                QString::fromUtf8(key.c_str()).split('.', Qt::SkipEmptyParts);

            if (parts.isEmpty())
                continue;

            QTreeWidgetItem* parentItem = nullptr;
            AZStd::string currentPath;

            for (const QString& token : parts)
            {
                if (!currentPath.empty())
                    currentPath += '.';
                currentPath += AZStd::string(token.toUtf8().constData());

                auto it = nodeMap.find(currentPath);
                if (it == nodeMap.end())
                {
                    QTreeWidgetItem* node = parentItem
                        ? new QTreeWidgetItem(parentItem,  QStringList(token))
                        : new QTreeWidgetItem(m_tree,      QStringList(token));

                    // Store full dot-path so any item can be selected as a key
                    node->setData(0, Qt::UserRole,
                        QString::fromUtf8(currentPath.c_str()));

                    nodeMap[currentPath] = node;
                    parentItem = node;
                }
                else
                {
                    parentItem = it->second;
                }
            }
        }

        m_tree->sortItems(0, Qt::AscendingOrder);
        m_tree->expandToDepth(0);
    }

    bool LexiconKeyPickerDialog::UpdateItemVisibility(
        QTreeWidgetItem* item, const QString& filter)
    {
        const QString fullPath = item->data(0, Qt::UserRole).toString();
        const bool selfMatches =
            fullPath.contains(filter, Qt::CaseInsensitive);

        bool anyChildMatches = false;
        for (int i = 0; i < item->childCount(); ++i)
        {
            if (UpdateItemVisibility(item->child(i), filter))
                anyChildMatches = true;
        }

        const bool visible = selfMatches || anyChildMatches;
        item->setHidden(!visible);

        if (anyChildMatches)
            item->setExpanded(true);

        return visible;
    }

} // namespace FoundationLocalisation
