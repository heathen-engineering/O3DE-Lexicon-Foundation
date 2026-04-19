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

#include <QMap>
#include <QStringList>
#include <QWidget>

class QComboBox;
class QFileSystemWatcher;
class QLabel;
class QLineEdit;
class QPushButton;
class QSplitter;
class QTableView;
class QTreeWidget;
class QTreeWidgetItem;

namespace FoundationLocalisation
{
    class LexiconGathererInboxWidget;
    class LexiconInspectorWidget;
    class LexiconTableModel;
    class LexiconTreeModel;

    ///<summary>
    /// "Lexicon Tool" — the main HeLex editor view pane.
    ///
    /// Layout:
    ///   Toolbar  — [Reference ▾] [Working ▾] | [⟳ Sync All] | [Export CSV] [Import CSV]
    ///   ┌──────────────┬──────────────────────────────┬───────────┐
    ///   │  [key field] │       Workbench              │ Inspector │
    ///   │  [+ Add Key] │ St │ Key │ Reference│Working │           │
    ///   │  Explorer    │                              │           │
    ///   └──────────────┴──────────────────────────────┴───────────┘
    ///   Gatherer Inbox
    ///
    /// Discovery:
    ///   On show, the window queries LexiconEditorRequestBus::GetKnownFilePaths()
    ///   for all .helex files found in the project. These populate the Reference
    ///   and Working dropdowns. The two dropdowns are mutually exclusive — selecting
    ///   a file in one removes it from the other.
    ///
    ///   Reference always has a "——" (no reference) option. When selected, the
    ///   Source column in the Workbench collapses and the Working column expands.
    ///
    /// Super-set sync:
    ///   On discovery (and after every Add Key / Remove Key), every discovered
    ///   .helex file is checked against the union of all keys. Any file missing
    ///   a key gets that key inserted as an empty string.
    ///</summary>
    class LexiconToolWindow : public QWidget
    {
        Q_OBJECT

    public:
        explicit LexiconToolWindow(QWidget* parent = nullptr);

    protected:
        void hideEvent(QHideEvent* event) override;
        void showEvent(QShowEvent* event) override;

    private slots:
        // Toolbar
        void OnReferenceChanged(int index);
        void OnWorkingChanged(int index);
        void OnNewLexicon();
        void OnSyncAll();
        void OnExportCsv();
        void OnImportCsv();
        void OnPreviewToggled(bool checked);

        // Explorer panel
        void OnTreeItemSelectionChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
        void OnTableSelectionChanged(const QModelIndex& current, const QModelIndex& previous);
        void OnFileChanged(const QString& path);
        void OnAddKey();

        // Workbench write-back
        void OnActiveValueEdited(const QString& key, const QString& newValue);

        // Gatherer Inbox write-back
        void OnGathererItemsAccepted(const QStringList& keys);

        // Inspector write-back
        void OnInspectorValueEdited(const QString& key, const QString& value);
        void OnInspectorBrowseAsset(const QString& key);

        // Context menu — remove
        void OnContextMenuRequested(const QPoint& pos);

    private:
        // ---- Layout helpers ----
        QWidget* MakeExplorerPanel();
        QWidget* MakePlaceholder(const QString& name, const QString& note);
        void     UpdateReferenceColumnVisibility();
        QWidget* MakeNodeButtons(const QString& fullPath, bool isLeaf, bool hasParent);

        // ---- Tag tree helpers ----
        void    RebuildTagTree();
        void    SelectTreeKey(const QString& path);
        QString GetSelectedTreeKey() const;
        void    ShowInspectorForKey(const QString& path);

        // ---- File operations ----
        /// Queries the editor bus for all discovered .helex paths and rebuilds dropdowns.
        void DiscoverFiles();

        /// Rebuilds both combo box contents, enforcing mutual exclusion.
        /// Call after m_discoveredFiles changes or after a selection changes.
        void RebuildDropdowns(const QString& keepReference, const QString& keepWorking);

        /// Checks all discovered files against the union of all keys; writes
        /// missing keys as "" into files that lack them. Returns number of files patched.
        int RunSuperSetSync();

        /// Writes key=value into every discovered .helex file that does not already
        /// contain that key. Used by Add Key (value="") and New Lexicon bootstrap.
        void WriteKeyToAllFiles(const QString& key, const QString& value = QString{});

        /// Removes a key from every discovered .helex file.
        void RemoveKeyFromAllFiles(const QString& key);

        /// Removes a set of keys from every discovered .helex file in a single
        /// read-modify-write pass per file. Used by namespace removal.
        void RemoveKeysFromAllFiles(const QStringList& keys);

        /// For each file in newFiles, injects Language.<otherAssetId> into that file
        /// for every other discovered lexicon, and Language.<newAssetId> into every
        /// other discovered file. Presents a diff-style confirmation before writing.
        /// Called by DiscoverFiles() when the workspace gains new .helex files.
        void RunCrossPollination(const QStringList& newFiles);

        /// Creates a minimal default.helex in the project root and rescans.
        /// Called automatically when no .helex files are found on discovery.
        void CreateDefaultLexicon();

        /// Writes value into the specified .helex file for the given key.
        /// Creates or updates the entry. Used by OnActiveValueEdited write-back.
        bool WriteValueToFile(const QString& filePath,
                              const QString& key,
                              const QJsonValue& value);

        /// Re-loads both tree and table from the current reference/working paths.
        void ReloadModels();

        // ---- Settings persistence ----
        void SaveSplitterState();
        void RestoreSplitterState();

        // ---- Toolbar widgets ----
        QComboBox*   m_referenceCombo = nullptr;
        QComboBox*   m_workingCombo   = nullptr;
        QToolButton* m_previewBtn     = nullptr;

        // Culture that was active before Live Preview was switched on.
        // Restored when the toggle is switched back off.
        QString m_previewPriorCulture;

        // ---- Explorer panel ----
        QTreeWidget* m_treeWidget     = nullptr;
        QLineEdit*   m_keyField       = nullptr;
        QPushButton* m_addKeyBtn      = nullptr;
        QLabel*      m_keyWarningLabel = nullptr;

        // ---- Workbench ----
        QTableView*         m_tableView  = nullptr;
        LexiconTableModel*  m_tableModel = nullptr;

        // ---- Tree model (shared by Explorer + Workbench) ----
        LexiconTreeModel* m_treeModel = nullptr;

        // ---- File watcher ----
        QFileSystemWatcher* m_fileWatcher    = nullptr;
        bool                m_syncingSelection = false;

        // ---- Other panels ----
        LexiconInspectorWidget*     m_inspector  = nullptr;
        LexiconGathererInboxWidget* m_inboxPanel = nullptr;

        // ---- Splitters ----
        QSplitter* m_verticalSplitter   = nullptr;
        QSplitter* m_horizontalSplitter = nullptr;

        // ---- Validation status bar ----
        QLabel* m_validationLabel = nullptr;

        // ---- Workspace state ----
        QStringList             m_discoveredFiles;   ///< all .helex source paths from last discovery
        QMap<QString, QString>  m_fileAssetIds;      ///< path → assetId label for dropdown display
        QString                 m_localisationDir;   ///< directory of default.helex; used as root for New browse

        static constexpr const char* SettingsGroup = "LexiconTool";
        static constexpr const char* VSplitterKey  = "VSplitterState";
        static constexpr const char* HSplitterKey  = "HSplitterState";

        // Sentinel stored as combo item data for the "——" no-reference option
        static const QString NoReference; // = ""
        // Sentinel stored as combo item data for the "New…" working option
        static const QString NewLexiconSentinel; // = "__new__"
    };

} // namespace FoundationLocalisation
