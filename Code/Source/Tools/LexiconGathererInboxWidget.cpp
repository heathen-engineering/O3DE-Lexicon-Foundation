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

#include "LexiconGathererInboxWidget.h"
#include "LexiconGathererWorker.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace FoundationLocalisation
{
    // Table column indices
    enum InboxColumn : int
    {
        IColCheck = 0,
        IColValue,
        IColKey,
        IColFile,
        IColCount
    };

    // Per-row metadata stored as UserRole data on the Key item
    static constexpr int RoleFilePath     = Qt::UserRole;
    static constexpr int RoleCurrentValue = Qt::UserRole + 1;

    ////////////////////////////////////////////////////////////////////////
    // Construction

    LexiconGathererInboxWidget::LexiconGathererInboxWidget(QWidget* parent)
        : QWidget(parent)
    {
        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(4, 4, 4, 4);
        root->setSpacing(4);

        // ---- Toolbar row ------------------------------------------------
        auto* toolbar = new QHBoxLayout();
        toolbar->setSpacing(4);

        auto* scanBtn = new QPushButton("Scan Now", this);
        scanBtn->setToolTip("Rescan project prefabs for Literal-mode Lexicon fields.");
        connect(scanBtn, &QPushButton::clicked,
                this, &LexiconGathererInboxWidget::OnScanNow);
        toolbar->addWidget(scanBtn);

        auto* sep1 = new QFrame(this);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);
        toolbar->addWidget(sep1);

        m_acceptBtn = new QPushButton("Accept Selected", this);
        m_acceptBtn->setToolTip(
            "Write checked entries to the target .helex and patch each source prefab.");
        m_acceptBtn->setEnabled(false);
        connect(m_acceptBtn, &QPushButton::clicked,
                this, &LexiconGathererInboxWidget::OnAcceptSelected);
        toolbar->addWidget(m_acceptBtn);

        m_rejectBtn = new QPushButton("Reject Selected", this);
        m_rejectBtn->setToolTip("Dismiss checked entries without writing them.");
        m_rejectBtn->setEnabled(false);
        connect(m_rejectBtn, &QPushButton::clicked,
                this, &LexiconGathererInboxWidget::OnRejectSelected);
        toolbar->addWidget(m_rejectBtn);

        toolbar->addStretch(1);

        m_statusLabel = new QLabel(this);
        m_statusLabel->setStyleSheet("color: #888; font-size: 10px;");
        toolbar->addWidget(m_statusLabel);

        root->addLayout(toolbar);

        // ---- Table -------------------------------------------------------
        m_table = new QTableWidget(0, IColCount, this);
        m_table->setHorizontalHeaderLabels(
            { QString{}, "Current Value", "Proposed Key", "Source File" });
        m_table->horizontalHeader()->setSectionResizeMode(IColCheck, QHeaderView::Fixed);
        m_table->horizontalHeader()->setSectionResizeMode(IColValue, QHeaderView::ResizeToContents);
        m_table->horizontalHeader()->setSectionResizeMode(IColKey,   QHeaderView::Stretch);
        m_table->horizontalHeader()->setSectionResizeMode(IColFile,  QHeaderView::ResizeToContents);
        m_table->setColumnWidth(IColCheck, 28);
        m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_table->setEditTriggers(
            QAbstractItemView::DoubleClicked | QAbstractItemView::SelectedClicked);
        m_table->verticalHeader()->setVisible(false);
        m_table->setMaximumHeight(160);
        // Re-evaluate Accept/Reject button state whenever a cell changes (e.g. checkbox toggled)
        connect(m_table, &QTableWidget::itemChanged,
                this, [this](QTableWidgetItem*) { UpdateButtons(); });
        root->addWidget(m_table, 1);

        // ---- Target .helex row -------------------------------------------
        auto* helexRow = new QHBoxLayout();
        helexRow->addWidget(new QLabel("Target .helex:", this));
        m_helexPath = new QLineEdit(this);
        m_helexPath->setPlaceholderText("Select or type the path to a .helex file\u2026");
        helexRow->addWidget(m_helexPath, 1);
        auto* browseBtn = new QPushButton("Browse\u2026", this);
        connect(browseBtn, &QPushButton::clicked,
                this, &LexiconGathererInboxWidget::OnBrowseHelex);
        helexRow->addWidget(browseBtn);
        root->addLayout(helexRow);

        setLayout(root);
    }

    ////////////////////////////////////////////////////////////////////////
    // Public

    void LexiconGathererInboxWidget::SetTargetHelexPath(const QString& path)
    {
        if (!path.isEmpty())
            m_helexPath->setText(path);
    }

    ////////////////////////////////////////////////////////////////////////
    // Private slots

    void LexiconGathererInboxWidget::OnScanNow()
    {
        const auto literals = LexiconGathererWorker::ScanPrefabs();
        PopulateTable(literals);
    }

    void LexiconGathererInboxWidget::OnAcceptSelected()
    {
        const QString helexQt = m_helexPath->text().trimmed();
        if (helexQt.isEmpty())
        {
            QMessageBox::warning(this, "No Target File",
                "Please choose a target .helex file before accepting entries.");
            return;
        }

        AZStd::vector<GatheredLiteral> confirmed;
        QStringList acceptedKeys;
        // Collect back-to-front so row removal indices stay valid
        QList<int> rowsToRemove;

        for (int row = 0; row < m_table->rowCount(); ++row)
        {
            auto* check = qobject_cast<QCheckBox*>(m_table->cellWidget(row, IColCheck));
            if (!check || !check->isChecked())
                continue;

            const QTableWidgetItem* keyItem = m_table->item(row, IColKey);
            const QString proposedKey = keyItem ? keyItem->text().trimmed() : QString{};
            if (proposedKey.isEmpty())
                continue;

            GatheredLiteral lit;
            lit.m_proposedKey  = AZStd::string(proposedKey.toUtf8().constData());
            lit.m_filePath     = AZStd::string(
                keyItem->data(RoleFilePath).toString().toUtf8().constData());
            lit.m_currentValue = AZStd::string(
                keyItem->data(RoleCurrentValue).toString().toUtf8().constData());

            confirmed.push_back(AZStd::move(lit));
            acceptedKeys.append(proposedKey);
            rowsToRemove.prepend(row); // prepend → descending order for removal
        }

        if (confirmed.empty())
        {
            QMessageBox::information(this, "Nothing Selected",
                "No entries are checked. Select at least one entry to accept.");
            return;
        }

        const AZStd::string helexPath(helexQt.toUtf8().constData());

        if (!LexiconGathererWorker::WriteToHelex(helexPath, confirmed))
        {
            QMessageBox::critical(this, "Write Failed",
                "Could not write to the .helex file.\n"
                "Check that the path is valid and writable.");
            return;
        }

        LexiconGathererWorker::PatchPrefabs(confirmed);

        for (int row : rowsToRemove)
            m_table->removeRow(row);

        m_statusLabel->setText(
            QString("Accepted %1 entr%2.")
                .arg(acceptedKeys.size())
                .arg(acceptedKeys.size() == 1 ? "y" : "ies"));

        UpdateButtons();
        emit ItemsAccepted(acceptedKeys);
    }

    void LexiconGathererInboxWidget::OnRejectSelected()
    {
        for (int row = m_table->rowCount() - 1; row >= 0; --row)
        {
            auto* check = qobject_cast<QCheckBox*>(m_table->cellWidget(row, IColCheck));
            if (check && check->isChecked())
                m_table->removeRow(row);
        }
        m_statusLabel->setText("Rejected selected entries.");
        UpdateButtons();
    }

    void LexiconGathererInboxWidget::OnBrowseHelex()
    {
        const QString path = QFileDialog::getSaveFileName(
            this, "Select Target .helex File",
            m_helexPath->text(),
            "Lexicon File (*.helex);;All Files (*)");
        if (!path.isEmpty())
            m_helexPath->setText(path);
    }

    ////////////////////////////////////////////////////////////////////////
    // Private helpers

    void LexiconGathererInboxWidget::PopulateTable(
        const AZStd::vector<GatheredLiteral>& literals)
    {
        m_table->setRowCount(0);
        m_statusLabel->clear();

        if (literals.empty())
        {
            m_statusLabel->setText("No Literal-mode Lexicon fields found in prefabs.");
            UpdateButtons();
            return;
        }

        m_table->setRowCount(static_cast<int>(literals.size()));

        for (int row = 0; row < static_cast<int>(literals.size()); ++row)
        {
            const GatheredLiteral& lit = literals[static_cast<size_t>(row)];

            // Col 0 — checkbox (checked by default)
            auto* check = new QCheckBox(m_table);
            check->setChecked(true);
            check->setStyleSheet("margin-left: 4px;");
            m_table->setCellWidget(row, IColCheck, check);

            // Col 1 — current value (read-only)
            auto* valItem = new QTableWidgetItem(
                QString::fromUtf8(lit.m_currentValue.c_str()));
            valItem->setFlags(valItem->flags() & ~Qt::ItemIsEditable);
            m_table->setItem(row, IColValue, valItem);

            // Col 2 — proposed key (editable); file path + current value stored as roles
            auto* keyItem = new QTableWidgetItem(
                QString::fromUtf8(lit.m_proposedKey.c_str()));
            keyItem->setData(RoleFilePath,
                QString::fromUtf8(lit.m_filePath.c_str()));
            keyItem->setData(RoleCurrentValue,
                QString::fromUtf8(lit.m_currentValue.c_str()));
            m_table->setItem(row, IColKey, keyItem);

            // Col 3 — source filename (read-only; tooltip = full path)
            const QString fullPath = QString::fromUtf8(lit.m_filePath.c_str());
            auto* fileItem = new QTableWidgetItem(QFileInfo(fullPath).fileName());
            fileItem->setFlags(fileItem->flags() & ~Qt::ItemIsEditable);
            fileItem->setToolTip(fullPath);
            m_table->setItem(row, IColFile, fileItem);
        }

        m_statusLabel->setText(
            QString("%1 item%2 pending.")
                .arg(static_cast<int>(literals.size()))
                .arg(literals.size() == 1 ? "" : "s"));

        UpdateButtons();
    }

    void LexiconGathererInboxWidget::UpdateButtons()
    {
        bool anyChecked = false;
        for (int row = 0; row < m_table->rowCount(); ++row)
        {
            const auto* check = qobject_cast<const QCheckBox*>(
                m_table->cellWidget(row, IColCheck));
            if (check && check->isChecked())
            {
                anyChecked = true;
                break;
            }
        }
        m_acceptBtn->setEnabled(anyChecked);
        m_rejectBtn->setEnabled(anyChecked);
    }

    ////////////////////////////////////////////////////////////////////////

} // namespace FoundationLocalisation

#include <moc_LexiconGathererInboxWidget.cpp>
