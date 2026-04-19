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

#pragma once

#include <QStringList>
#include <QWidget>

// AZ/AZStd headers confuse Qt MOC — guard them so MOC only sees forward decls.
#ifndef Q_MOC_RUN
#  include "LexiconGathererWorker.h"
#else
namespace FoundationLocalisation { struct GatheredLiteral; }
namespace AZStd { template<class T> class vector; }
#endif

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;

namespace FoundationLocalisation
{
    ///<summary>
    /// Persistent Gatherer Inbox panel — the bottom strip of the Lexicon Tool.
    ///
    /// Replaces the former modal LexiconGathererDialog (Phase 5) with a non-blocking,
    /// always-visible panel. The user can scan, review, edit proposed keys, and
    /// accept or reject rows without leaving the tool window.
    ///
    /// Layout:
    ///   [Scan Now] [Accept Selected] [Reject Selected]  ← status text
    ///   ┌─┬──────────────┬────────────────────┬────────────┐
    ///   │☑│ Current Value│ Proposed Key (edit) │ Source File│
    ///   └─┴──────────────┴────────────────────┴────────────┘
    ///   Target .helex: [path field ····] [Browse…]
    ///
    /// Accept flow:
    ///   1. Collect checked rows with (possibly user-edited) proposed keys.
    ///   2. Write entries to the target .helex via LexiconGathererWorker::WriteToHelex.
    ///   3. Patch prefabs via LexiconGathererWorker::PatchPrefabs.
    ///   4. Remove accepted rows; emit ItemsAccepted so the tool window syncs + reloads.
    ///
    /// Reject flow:
    ///   Remove checked rows without writing anything.
    ///
    /// SetTargetHelexPath is called by LexiconToolWindow whenever the Working
    /// dropdown changes, so the inbox defaults to the file being actively edited.
    ///
    /// This widget is pure Qt — no O3DE headers outside the MOC guard.
    ///</summary>
    class LexiconGathererInboxWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit LexiconGathererInboxWidget(QWidget* parent = nullptr);

        ///<summary>
        /// Pre-fills the target .helex path field.
        /// Called by LexiconToolWindow when the Working dropdown selection changes.
        ///</summary>
        void SetTargetHelexPath(const QString& path);

    signals:
        ///<summary>
        /// Emitted after accepted items are written to the .helex file and prefabs patched.
        /// keys holds the dot-path keys that were added; LexiconToolWindow connects this
        /// to RunSuperSetSync() + ReloadModels().
        ///</summary>
        void ItemsAccepted(const QStringList& keys);

    private slots:
        void OnScanNow();
        void OnAcceptSelected();
        void OnRejectSelected();
        void OnBrowseHelex();

    private:
        void PopulateTable(const AZStd::vector<GatheredLiteral>& literals);
        void UpdateButtons();

        QTableWidget* m_table       = nullptr;
        QLabel*       m_statusLabel = nullptr;
        QLineEdit*    m_helexPath   = nullptr;
        QPushButton*  m_acceptBtn   = nullptr;
        QPushButton*  m_rejectBtn   = nullptr;
    };

} // namespace FoundationLocalisation
