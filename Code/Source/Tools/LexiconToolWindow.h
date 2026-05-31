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

#include "LexiconEntryMap.h"
#include "LexiconTableModel.h"

#include <FoundationLocalisation/LexiconHintType.h>

#include <QMap>
#include <QStringList>
#include <QWidget>

class QFileSystemWatcher;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QSplitter;
class QStackedWidget;
class QTabBar;
class QTableView;
class QTextEdit;
class QTimer;
class QToolButton;
class QVBoxLayout;

namespace FoundationLocalisation
{
    class LexiconGathererInboxWidget;
    class LexiconTableModel;
    class LexiconTreeModel;

    ///<summary>
    /// "Lexicon Tool" — the Heathen Localisation Lexicon editor view pane.
    ///
    /// Layout (matches Unity's Localisation Lexicon Project Settings page):
    ///
    ///  ▾ Source Files
    ///  ┌──────────────────────────────────────────────────────────────────────┐
    ///  │  default               [en ✕] [fr ✕]                               │
    ///  │  [type to search...                                       ] [+]     │
    ///  │  OtherCulture          [de ✕]                                       │
    ///  │  [type to search...                                       ] [+]     │
    ///  │  [New Culture File…]                                                │
    ///  └──────────────────────────────────────────────────────────────────────┘
    ///  [ Workbench ]  [ Gather ]  [ CSV ]
    ///  ┌─────────────────┬────────────────────────────────────────────────────┐
    ///  │  [key field  ]+│  ✕ │ Key          │ Type   │ Default │ OtherCulture│
    ///  │  Key tree      │  ✕ │ UI.Title     │ String │ My Game │ Mein Spiel  │
    ///  └─────────────────┴────────────────────────────────────────────────────┘
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
        // Source Files panel
        void OnNewCultureFile();
        void OnToggleSourceFiles();

        // Add Key bar
        void OnAddKey();
        void OnKeyFieldChanged(const QString& text);

        // Table signals from LexiconTableModel
        void OnTypeChanged(const QString& key, LexiconHintType newHint);
        void OnDefaultValueEdited(const QString& key, const QString& newValue);
        void OnExtraValueEdited(const QString& key, int extraColIndex, const QString& newValue);

        // File watcher
        void OnFileChanged(const QString& path);

        // Tab switching
        void OnTabChanged(int index);

        // Table context menu (right-click → delete key)
        void OnTableContextMenuRequested(const QPoint& pos);

        // Gatherer
        void OnGathererItemsAccepted(const QStringList& keys);

        // CSV tab
        void OnCsvExport();
        void OnCsvImport();
        void OnCsvCopyToClipboard();

    private:
        // ── Layout builders ───────────────────────────────────────────────────
        QWidget* BuildSourceFilesPanel();
        QWidget* BuildWorkbenchTab();
        QWidget* BuildGatherTab();
        QWidget* BuildCsvTab();

        // ── Source Files panel ────────────────────────────────────────────────
        void RebuildSourceFileEntries();
        void AddCultureToFile(int fileIdx, const QString& code);
        void RemoveCultureFromFile(int fileIdx, const QString& code);
        void ShowCulturePicker(int fileIdx, const QString& filter);
        void AddExtraColumn(const QString& filePath);
        void RemoveExtraColumn(const QString& filePath);
        void ShowExtraColumnPicker();

        // ── File operations ───────────────────────────────────────────────────
        void DiscoverFiles();
        int  RunSuperSetSync();
        void WriteKeyToAllFiles(const QString& key, const QString& value = {});
        void RemoveKeyFromAllFiles(const QString& key);
        void RemoveKeysFromAllFiles(const QStringList& keys);
        void RunCrossPollination(const QStringList& newFiles);
        void CreateDefaultLexicon();
        bool WriteValueToFile(const QString& filePath, const QString& key,
                              const QJsonValue& value);
        void WriteTypeToFile(const QString& filePath, const QString& key,
                             LexiconHintType hint);
        void ReloadModels();

        // ── Column layout ─────────────────────────────────────────────────────
        void ApplyValueColumnModes();

        // ── Settings persistence ──────────────────────────────────────────────
        void SaveState();
        void RestoreState();

        // ── Source Files panel ────────────────────────────────────────────────
        QWidget*     m_sourcesPanel      = nullptr;   ///< header row (always visible)
        QScrollArea* m_sourcesScrollArea = nullptr;   ///< scrollable file list
        QVBoxLayout* m_sourcesBodyLayout = nullptr;
        QToolButton* m_sourcesToggle     = nullptr;
        QWidget*     m_sourcesBody       = nullptr;
        bool         m_sourcesExpanded   = true;

        // ── Sources ↕ Tabs splitter ───────────────────────────────────────────
        QSplitter*    m_sourcesSplitter = nullptr;

        // ── Tab bar + stacked content ─────────────────────────────────────────
        QTabBar*        m_tabBar   = nullptr;
        QStackedWidget* m_tabStack = nullptr;

        // ── Workbench tab ─────────────────────────────────────────────────────
        QLineEdit*   m_keyField        = nullptr;
        QPushButton* m_addKeyBtn       = nullptr;
        QLabel*      m_keyWarningLabel = nullptr;
        QTableView*  m_tableView       = nullptr;

        LexiconTreeModel*  m_treeModel  = nullptr;  ///< data loading only (not displayed)
        LexiconTableModel* m_tableModel = nullptr;

        // ── CSV tab ───────────────────────────────────────────────────────────
        QTextEdit* m_csvPreview = nullptr;

        // ── Gather tab ────────────────────────────────────────────────────────
        LexiconGathererInboxWidget* m_inboxPanel = nullptr;

        // ── File watcher ──────────────────────────────────────────────────────
        QFileSystemWatcher* m_fileWatcher = nullptr;

        // ── Workspace state ───────────────────────────────────────────────────
        struct HelexFileEntry
        {
            QString     path;
            QString     assetId;
            QStringList cultures;
        };
        QVector<HelexFileEntry> m_fileEntries;       ///< all discovered files + metadata
        QStringList             m_discoveredFiles;   ///< all .helex source paths
        QMap<QString, QString>  m_fileAssetIds;      ///< path → assetId label
        QString                 m_localisationDir;   ///< directory of default.helex

        QStringList m_extraColumnPaths;  ///< extra culture file paths added to workbench table

        static constexpr const char* SettingsGroup  = "HeathenEditor/LexiconTool";

        static const QString NewLexiconSentinel;
    };

} // namespace FoundationLocalisation
