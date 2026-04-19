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

#include "LexiconToolWindow.h"
#include "LexiconCsvInterop.h"
#include "LexiconEditorRequestBus.h"
#include "LexiconGathererInboxWidget.h"
#include "LexiconInspectorWidget.h"
#include "LexiconNewFileDialog.h"
#include "LexiconTableModel.h"
#include "LexiconTreeModel.h"
#include "LexiconValidator.h"
#include "LexiconValueDelegate.h"

#include <FoundationLocalisation/FoundationLocalisationBus.h>
#include <FoundationLocalisation/LexiconAssemblyAsset.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>


#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QGroupBox>
#include <QRadioButton>
#include <QSet>
#include <QTextEdit>
#include <QTextStream>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QFileSystemWatcher>
#include <QTableView>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>

namespace FoundationLocalisation
{
    // Static sentinel values stored as combo item user data
    const QString LexiconToolWindow::NoReference        = QString{};
    const QString LexiconToolWindow::NewLexiconSentinel = QStringLiteral("__new__");

    static QString ResolveAssetPath(const QString& uuidStr)
    {
        const AZ::Uuid uuid = AZ::Uuid::CreateString(uuidStr.toUtf8().constData());
        if (uuid.IsNull())
            return {};

        AZStd::string relativePath;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            relativePath,
            &AZ::Data::AssetCatalogRequests::GetAssetPathById,
            AZ::Data::AssetId(uuid, 0));

        return QString::fromUtf8(relativePath.c_str());
    }

    ////////////////////////////////////////////////////////////////////////
    // Construction

    LexiconToolWindow::LexiconToolWindow(QWidget* parent)
        : QWidget(parent)
    {
        setObjectName("LexiconToolWindow");

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        // ---- Toolbar --------------------------------------------------------
        auto* toolbarFrame = new QFrame(this);
        toolbarFrame->setFrameShape(QFrame::StyledPanel);
        toolbarFrame->setFixedHeight(34);

        auto* toolbarLayout = new QHBoxLayout(toolbarFrame);
        toolbarLayout->setContentsMargins(4, 2, 4, 2);
        toolbarLayout->setSpacing(4);

        // Reference dropdown
        auto* refLabel = new QLabel("Reference:", toolbarFrame);
        toolbarLayout->addWidget(refLabel);

        m_referenceCombo = new QComboBox(toolbarFrame);
        m_referenceCombo->setToolTip(
            "Read-only comparison column.\n"
            "Select '——' to work without a reference (column hidden).");
        m_referenceCombo->setMinimumWidth(140);
        connect(m_referenceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &LexiconToolWindow::OnReferenceChanged);
        toolbarLayout->addWidget(m_referenceCombo);

        auto* sep1 = new QFrame(toolbarFrame);
        sep1->setFrameShape(QFrame::VLine);
        sep1->setFrameShadow(QFrame::Sunken);
        toolbarLayout->addWidget(sep1);

        // Working dropdown
        auto* workLabel = new QLabel("Working:", toolbarFrame);
        toolbarLayout->addWidget(workLabel);

        m_workingCombo = new QComboBox(toolbarFrame);
        m_workingCombo->setToolTip("The .helex file being translated / edited.");
        m_workingCombo->setMinimumWidth(140);
        connect(m_workingCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &LexiconToolWindow::OnWorkingChanged);
        toolbarLayout->addWidget(m_workingCombo);

        toolbarLayout->addStretch(1);

        auto* sep2 = new QFrame(toolbarFrame);
        sep2->setFrameShape(QFrame::VLine);
        sep2->setFrameShadow(QFrame::Sunken);
        toolbarLayout->addWidget(sep2);

        // Sync All
        auto* syncBtn = new QToolButton(toolbarFrame);
        syncBtn->setText("\u27F3 Sync All");
        syncBtn->setToolTip(
            "Check all discovered .helex files against the union of all keys.\n"
            "Any file missing a key gets it inserted as an empty string.");
        connect(syncBtn, &QToolButton::clicked, this, &LexiconToolWindow::OnSyncAll);
        toolbarLayout->addWidget(syncBtn);

        auto* sep3 = new QFrame(toolbarFrame);
        sep3->setFrameShape(QFrame::VLine);
        sep3->setFrameShadow(QFrame::Sunken);
        toolbarLayout->addWidget(sep3);

        // CSV interop
        auto* exportBtn = new QToolButton(toolbarFrame);
        exportBtn->setText("Export CSV");
        exportBtn->setToolTip(
            "Export lexicon data to a CSV file.\n"
            "Single: exports the Working lexicon only.\n"
            "Multi: exports every discovered lexicon as side-by-side columns.");
        connect(exportBtn, &QToolButton::clicked, this, &LexiconToolWindow::OnExportCsv);
        toolbarLayout->addWidget(exportBtn);

        auto* importBtn = new QToolButton(toolbarFrame);
        importBtn->setText("Import CSV");
        importBtn->setToolTip(
            "Import translated values from a CSV file.\n"
            "Multi (default): each column maps to the .helex named in Row 1.\n"
            "Single: reads only the first value column into a chosen lexicon.");
        connect(importBtn, &QToolButton::clicked, this, &LexiconToolWindow::OnImportCsv);
        toolbarLayout->addWidget(importBtn);

        auto* sep4 = new QFrame(toolbarFrame);
        sep4->setFrameShape(QFrame::VLine);
        sep4->setFrameShadow(QFrame::Sunken);
        toolbarLayout->addWidget(sep4);

        // Live Preview toggle
        m_previewBtn = new QToolButton(toolbarFrame);
        m_previewBtn->setText("\u25CE Preview");
        m_previewBtn->setToolTip(
            "Hot-swap the active culture in the editor viewport to match the Working lexicon.\n"
            "Requires the Asset Processor to have built the .lexicon product.\n"
            "Restores the previous culture when toggled off.");
        m_previewBtn->setCheckable(true);
        m_previewBtn->setChecked(false);
        connect(m_previewBtn, &QToolButton::toggled, this, &LexiconToolWindow::OnPreviewToggled);
        toolbarLayout->addWidget(m_previewBtn);

        root->addWidget(toolbarFrame);

        // ---- Shared models --------------------------------------------------
        m_treeModel  = new LexiconTreeModel(this);
        m_tableModel = new LexiconTableModel(this);

        // ---- Main content area ----------------------------------------------
        m_verticalSplitter = new QSplitter(Qt::Vertical, this);
        m_verticalSplitter->setChildrenCollapsible(false);

        m_horizontalSplitter = new QSplitter(Qt::Horizontal, m_verticalSplitter);
        m_horizontalSplitter->setChildrenCollapsible(false);

        // Explorer panel (Add Key bar + tree view)
        QWidget* explorerPanel = MakeExplorerPanel();
        explorerPanel->setMinimumWidth(180);

        // Workbench — translation grid
        m_tableView = new QTableView(m_horizontalSplitter);
        m_tableView->setModel(m_tableModel);
        m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
        m_tableView->setItemDelegateForColumn(LexiconTableModel::ColActive, new LexiconValueDelegate(m_tableView));
        m_tableView->horizontalHeader()->setStretchLastSection(true);
        m_tableView->horizontalHeader()->setSectionResizeMode(LexiconTableModel::ColStatus, QHeaderView::Fixed);
        m_tableView->horizontalHeader()->setSectionResizeMode(LexiconTableModel::ColKey,    QHeaderView::Interactive);
        m_tableView->horizontalHeader()->setSectionResizeMode(LexiconTableModel::ColSource, QHeaderView::Interactive);
        m_tableView->horizontalHeader()->resizeSection(LexiconTableModel::ColStatus, 28);
        m_tableView->verticalHeader()->hide();
        m_tableView->setAlternatingRowColors(true);
        m_tableView->setShowGrid(false);
        m_tableView->setMinimumWidth(320);

        // Inspector panel
        m_inspector = new LexiconInspectorWidget(m_horizontalSplitter);
        m_inspector->setMinimumWidth(180);

        m_horizontalSplitter->addWidget(explorerPanel);
        m_horizontalSplitter->addWidget(m_tableView);
        m_horizontalSplitter->addWidget(m_inspector);
        m_horizontalSplitter->setStretchFactor(0, 0);
        m_horizontalSplitter->setStretchFactor(1, 1);
        m_horizontalSplitter->setStretchFactor(2, 0);

        // Gatherer Inbox
        m_inboxPanel = new LexiconGathererInboxWidget(m_verticalSplitter);
        m_inboxPanel->setMinimumHeight(80);
        m_inboxPanel->setMaximumHeight(220);

        m_verticalSplitter->addWidget(m_horizontalSplitter);
        m_verticalSplitter->addWidget(m_inboxPanel);
        m_verticalSplitter->setStretchFactor(0, 1);
        m_verticalSplitter->setStretchFactor(1, 0);

        root->addWidget(m_verticalSplitter, 1);

        // ---- Validation status bar ------------------------------------------
        m_validationLabel = new QLabel(this);
        m_validationLabel->setContentsMargins(6, 2, 6, 2);
        m_validationLabel->setStyleSheet("font-size: 11px; color: #a0a0a0;");
        m_validationLabel->setText("No files open.");
        root->addWidget(m_validationLabel);

        setLayout(root);

        // ---- File watcher ---------------------------------------------------
        m_fileWatcher = new QFileSystemWatcher(this);
        connect(m_fileWatcher, &QFileSystemWatcher::fileChanged,
                this, &LexiconToolWindow::OnFileChanged);

        // ---- Signal wiring --------------------------------------------------
        // Tree selection → update inspector + scroll table
        connect(m_treeWidget, &QTreeWidget::currentItemChanged,
                this, &LexiconToolWindow::OnTreeItemSelectionChanged);

        // Table selection → scroll tree to matching node (bidirectional)
        connect(m_tableView->selectionModel(), &QItemSelectionModel::currentRowChanged,
                this, &LexiconToolWindow::OnTableSelectionChanged);

        // Workbench edit → file write-back
        connect(m_tableModel, &LexiconTableModel::ActiveValueEdited,
                this, &LexiconToolWindow::OnActiveValueEdited);

        // Inspector edit → file write-back + asset browse
        connect(m_inspector, &LexiconInspectorWidget::ValueEdited,
                this, &LexiconToolWindow::OnInspectorValueEdited);
        connect(m_inspector, &LexiconInspectorWidget::BrowseAssetRequested,
                this, &LexiconToolWindow::OnInspectorBrowseAsset);

        // Gatherer Inbox → sync + reload after accepted items are written
        connect(m_inboxPanel, &LexiconGathererInboxWidget::ItemsAccepted,
                this, &LexiconToolWindow::OnGathererItemsAccepted);

        RestoreSplitterState();
    }

    ////////////////////////////////////////////////////////////////////////
    // Show / Hide

    void LexiconToolWindow::hideEvent(QHideEvent* event)
    {
        SaveSplitterState();
        QWidget::hideEvent(event);
    }

    void LexiconToolWindow::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        RestoreSplitterState();
        DiscoverFiles();
    }

    ////////////////////////////////////////////////////////////////////////
    // Toolbar slots

    void LexiconToolWindow::OnReferenceChanged(int /*index*/)
    {
        const QString refPath  = m_referenceCombo->currentData().toString();
        const QString workPath = m_workingCombo->currentData().toString();

        // Exclude the newly-selected reference from Working options
        RebuildDropdowns(refPath, workPath);
        UpdateReferenceColumnVisibility();

        // Route through ReloadModels so validation runs and the table is updated
        ReloadModels();
    }

    void LexiconToolWindow::OnWorkingChanged(int /*index*/)
    {
        const QString workPath = m_workingCombo->currentData().toString();

        // "New…" sentinel — open dialog then reset
        if (workPath == NewLexiconSentinel)
        {
            m_workingCombo->blockSignals(true);
            m_workingCombo->setCurrentIndex(0);
            m_workingCombo->blockSignals(false);
            OnNewLexicon();
            return;
        }

        // Switching the working file while preview is on would preview the wrong culture;
        // turn it off first so OnPreviewToggled restores the saved culture.
        if (m_previewBtn->isChecked())
            m_previewBtn->setChecked(false);

        const QString refPath = m_referenceCombo->currentData().toString();
        RebuildDropdowns(refPath, workPath);
        UpdateReferenceColumnVisibility();

        // Route through ReloadModels so validation runs and the table is updated
        ReloadModels();

        // Keep the Gatherer Inbox defaulting to the file being edited
        if (workPath != NewLexiconSentinel && !workPath.isEmpty())
            m_inboxPanel->SetTargetHelexPath(workPath);

        const bool anyFiles = !m_discoveredFiles.isEmpty();
        m_addKeyBtn->setEnabled(anyFiles);
        m_keyField->setEnabled(anyFiles);
    }

    void LexiconToolWindow::OnNewLexicon()
    {
        // Use the directory of default.helex as the browse root.
        // If not yet discovered, fall back to querying the bus.
        QString browseDir = m_localisationDir;
        if (browseDir.isEmpty())
        {
            AZStd::string sourcePath;
            LexiconEditorRequestBus::BroadcastResult(
                sourcePath, &LexiconEditorRequests::GetProjectSourcePath);
            browseDir = QString::fromUtf8(sourcePath.c_str());
            if (!browseDir.endsWith(QLatin1Char('/')))
                browseDir += QLatin1Char('/');
            browseDir += QLatin1String("Assets/Localisation");
        }

        LexiconNewFileDialog dlg(browseDir, this);
        if (dlg.exec() != QDialog::Accepted)
            return;

        const QString path        = dlg.FilePath();
        const QString assetId     = dlg.AssetId();
        const QString displayName = dlg.DisplayName();
        const QString cultures    = dlg.CultureCodes();

        // Build cultures array
        QJsonArray culturesArray;
        for (const QString& code : cultures.split(QLatin1Char(','), Qt::SkipEmptyParts))
            culturesArray.append(code.trimmed());

        // Build initial entries — Language.<assetId> only; super-set sync fills the rest
        QJsonObject entries;
        if (!displayName.isEmpty())
            entries[QStringLiteral("Language.") + assetId] = displayName;

        QJsonObject root;
        root[QLatin1String("assetId")]  = assetId;
        root[QLatin1String("cultures")] = culturesArray;
        root[QLatin1String("entries")]  = entries;

        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            QMessageBox::warning(this, "New Lexicon Failed",
                QString("Could not write file:\n%1").arg(path));
            return;
        }
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();

        // Ask the EditorSystemComponent to re-scan so it picks up the new file,
        // then discover again so the dropdowns update.
        LexiconEditorRequestBus::Broadcast(&LexiconEditorRequests::RefreshKeyTree);
        DiscoverFiles();

        // Select the new file as Working
        const int idx = m_workingCombo->findData(path);
        if (idx >= 0)
            m_workingCombo->setCurrentIndex(idx);

        // Super-set sync fills the new file with all existing keys
        RunSuperSetSync();
        ReloadModels();
    }

    void LexiconToolWindow::OnSyncAll()
    {
        const int patched = RunSuperSetSync();
        if (patched > 0)
        {
            ReloadModels();
            QMessageBox::information(this, "Sync Complete",
                QString("Super-set sync patched %1 file(s).").arg(patched));
        }
        else
        {
            QMessageBox::information(this, "Sync Complete",
                "All files already have the complete key set.");
        }
    }

    void LexiconToolWindow::OnExportCsv()
    {
        if (m_discoveredFiles.isEmpty())
        {
            QMessageBox::information(this, "Export CSV",
                "No .helex files found in the project.");
            return;
        }

        // ---- Mode selection dialog ----
        QDialog modeDlg(this);
        modeDlg.setWindowTitle("Export CSV — Mode");
        auto* modeLayout = new QVBoxLayout(&modeDlg);

        auto* modeGroup  = new QGroupBox("Export mode", &modeDlg);
        auto* modeInner  = new QVBoxLayout(modeGroup);

        auto* singleRadio = new QRadioButton(
            "Single — export the Working lexicon only", modeGroup);
        auto* multiRadio  = new QRadioButton(
            "Multi  — export every discovered lexicon as side-by-side columns", modeGroup);
        multiRadio->setChecked(true);

        modeInner->addWidget(singleRadio);
        modeInner->addWidget(multiRadio);

        auto* noteLabel = new QLabel(
            "CSV structure:\n"
            "  Row 0 — column headers\n"
            "  Row 1 — .helex file names (identity row)\n"
            "  Row 2+ — key, value(s)",
            &modeDlg);
        noteLabel->setStyleSheet("font-size: 10px; color: #808080;");

        auto* buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &modeDlg);
        connect(buttons, &QDialogButtonBox::accepted, &modeDlg, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &modeDlg, &QDialog::reject);

        modeLayout->addWidget(modeGroup);
        modeLayout->addWidget(noteLabel);
        modeLayout->addWidget(buttons);

        if (modeDlg.exec() != QDialog::Accepted)
            return;

        const bool isSingle = singleRadio->isChecked();

        if (isSingle)
        {
            // Single requires a Working file
            const QString workPath = m_workingCombo->currentData().toString();
            if (workPath.isEmpty() || workPath == NewLexiconSentinel)
            {
                QMessageBox::warning(this, "Export CSV",
                    "Please select a Working lexicon before using Single export.");
                return;
            }

            QString savePath = QFileDialog::getSaveFileName(
                this,
                "Export CSV — Single",
                m_localisationDir,
                "CSV Files (*.csv)");
            if (savePath.isEmpty())
                return;
            if (!savePath.endsWith(QLatin1String(".csv"), Qt::CaseInsensitive))
                savePath += QLatin1String(".csv");

            // Use the active-file values from the current tree model entries.
            const QString error = LexiconCsvInterop::ExportSingle(
                savePath, m_treeModel->EntryMap(), workPath);

            if (!error.isEmpty())
                QMessageBox::warning(this, "Export Failed", error);
            else
                QMessageBox::information(this, "Export CSV",
                    QString("Single export written to:\n%1").arg(savePath));
        }
        else
        {
            // Multi — load every discovered file into its own entry map.
            QString savePath = QFileDialog::getSaveFileName(
                this,
                "Export CSV — Multi",
                m_localisationDir,
                "CSV Files (*.csv)");
            if (savePath.isEmpty())
                return;
            if (!savePath.endsWith(QLatin1String(".csv"), Qt::CaseInsensitive))
                savePath += QLatin1String(".csv");

            QVector<LexiconEntryMap> maps;
            maps.reserve(m_discoveredFiles.size());

            for (const QString& path : m_discoveredFiles)
            {
                LexiconEntryMap fileMap;
                QFile f(path);
                if (f.open(QIODevice::ReadOnly))
                {
                    QJsonParseError err;
                    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
                    f.close();
                    if (err.error == QJsonParseError::NoError && doc.isObject())
                    {
                        const QJsonObject entries =
                            doc.object().value(QLatin1String("entries")).toObject();
                        for (auto it = entries.begin(); it != entries.end(); ++it)
                        {
                            LexiconEntry entry;
                            if (it.value().isObject())
                            {
                                // Asset reference — store UUID string as the value
                                entry.activeValue = it.value().toObject()
                                    .value(QLatin1String("uuid")).toString();
                                entry.isAsset = true;
                            }
                            else
                            {
                                entry.activeValue = it.value().toString();
                            }
                            fileMap.insert(it.key(), entry);
                        }
                    }
                }
                maps.append(fileMap);
            }

            const QString error = LexiconCsvInterop::ExportMulti(
                savePath, m_discoveredFiles, maps);

            if (!error.isEmpty())
                QMessageBox::warning(this, "Export Failed", error);
            else
                QMessageBox::information(this, "Export CSV",
                    QString("Multi export (%1 lexicon(s)) written to:\n%2")
                        .arg(m_discoveredFiles.size())
                        .arg(savePath));
        }
    }

    void LexiconToolWindow::OnImportCsv()
    {
        // ---- File picker first so the user can cancel cheaply ----
        const QString csvPath = QFileDialog::getOpenFileName(
            this,
            "Import CSV",
            m_localisationDir,
            "CSV Files (*.csv);;All Files (*)");
        if (csvPath.isEmpty())
            return;

        // ---- Mode selection dialog ----
        QDialog modeDlg(this);
        modeDlg.setWindowTitle("Import CSV — Mode");
        auto* modeLayout = new QVBoxLayout(&modeDlg);

        auto* modeGroup  = new QGroupBox("Import mode", &modeDlg);
        auto* modeInner  = new QVBoxLayout(modeGroup);

        auto* multiRadio  = new QRadioButton(
            "Multi  — read every column; map each to the .helex named in Row 1 (default)",
            modeGroup);
        auto* singleRadio = new QRadioButton(
            "Single — read only the first value column into a chosen lexicon",
            modeGroup);
        multiRadio->setChecked(true);

        modeInner->addWidget(multiRadio);
        modeInner->addWidget(singleRadio);

        auto* noteLabel = new QLabel(
            "Multi will create a new .helex next to default.helex\n"
            "if the file name in Row 1 does not match a known file.",
            &modeDlg);
        noteLabel->setStyleSheet("font-size: 10px; color: #808080;");

        auto* buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &modeDlg);
        connect(buttons, &QDialogButtonBox::accepted, &modeDlg, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &modeDlg, &QDialog::reject);

        modeLayout->addWidget(modeGroup);
        modeLayout->addWidget(noteLabel);
        modeLayout->addWidget(buttons);

        if (modeDlg.exec() != QDialog::Accepted)
            return;

        if (multiRadio->isChecked())
        {
            // ---- Multi import ----
            const LexiconCsvInterop::MultiImportResult result =
                LexiconCsvInterop::ParseMulti(csvPath);

            if (!result.errors.isEmpty())
            {
                const auto answer = QMessageBox::question(
                    this, "Import CSV — Parse Warnings",
                    QString("%1 warning(s) encountered while parsing:\n\n%2\n\nContinue with import?")
                        .arg(result.errors.size())
                        .arg(result.errors.join(QLatin1Char('\n'))),
                    QMessageBox::Yes | QMessageBox::Cancel,
                    QMessageBox::Cancel);
                if (answer != QMessageBox::Yes)
                    return;
            }

            if (result.columns.isEmpty())
            {
                QMessageBox::information(this, "Import CSV",
                    "No value columns found in the file.");
                return;
            }

            // Resolve each column's file name to a full path.
            // If not found in discovered files, create a new .helex next to default.helex.
            int filesWritten  = 0;
            int keysImported  = 0;
            QStringList createdFiles;

            for (const LexiconCsvInterop::MultiImportResult::Column& col : result.columns)
            {
                if (col.filePath.isEmpty() || col.pairs.isEmpty())
                    continue;

                // Try to match by file name (basename) against discovered files
                QString targetPath;
                for (const QString& dp : m_discoveredFiles)
                {
                    if (QFileInfo(dp).fileName().compare(
                            col.filePath, Qt::CaseInsensitive) == 0)
                    {
                        targetPath = dp;
                        break;
                    }
                }

                if (targetPath.isEmpty())
                {
                    // Create a new .helex next to default.helex using the Row 1 name
                    if (m_localisationDir.isEmpty())
                    {
                        QMessageBox::warning(this, "Import CSV",
                            QString("Cannot create '%1': localisation directory not found.")
                                .arg(col.filePath));
                        continue;
                    }

                    QString fileName = col.filePath;
                    if (!fileName.endsWith(QLatin1String(".helex"), Qt::CaseInsensitive))
                        fileName += QLatin1String(".helex");

                    targetPath = m_localisationDir + QLatin1Char('/') + fileName;

                    const QString baseName = QFileInfo(targetPath).completeBaseName();
                    QJsonObject newRoot;
                    newRoot[QLatin1String("assetId")]  = baseName;
                    newRoot[QLatin1String("cultures")] = QJsonArray{};
                    newRoot[QLatin1String("entries")]  = QJsonObject{};

                    QFile nf(targetPath);
                    if (!nf.open(QIODevice::WriteOnly | QIODevice::Truncate))
                    {
                        QMessageBox::warning(this, "Import CSV",
                            QString("Could not create file:\n%1").arg(targetPath));
                        continue;
                    }
                    nf.write(QJsonDocument(newRoot).toJson(QJsonDocument::Indented));
                    nf.close();
                    createdFiles << targetPath;
                }

                // Write all pairs into the target file
                for (auto it = col.pairs.cbegin(); it != col.pairs.cend(); ++it)
                {
                    WriteValueToFile(targetPath, it.key(), QJsonValue(it.value()));
                    ++keysImported;
                }
                ++filesWritten;
            }

            // Rescan if new files were created, then sync and reload
            if (!createdFiles.isEmpty())
            {
                LexiconEditorRequestBus::Broadcast(&LexiconEditorRequests::RefreshKeyTree);
                DiscoverFiles();
            }
            else
            {
                RunSuperSetSync();
                ReloadModels();
            }

            QString summary = QString("Imported %1 key(s) into %2 file(s).")
                .arg(keysImported).arg(filesWritten);
            if (!createdFiles.isEmpty())
                summary += QString("\n\nCreated %1 new lexicon(s):\n%2")
                    .arg(createdFiles.size())
                    .arg(createdFiles.join(QLatin1Char('\n')));

            QMessageBox::information(this, "Import CSV", summary);
        }
        else
        {
            // ---- Single import ----
            const LexiconCsvInterop::SingleImportResult result =
                LexiconCsvInterop::ParseSingle(csvPath);

            if (!result.errors.isEmpty())
            {
                const auto answer = QMessageBox::question(
                    this, "Import CSV — Parse Warnings",
                    QString("%1 warning(s) encountered while parsing:\n\n%2\n\nContinue with import?")
                        .arg(result.errors.size())
                        .arg(result.errors.join(QLatin1Char('\n'))),
                    QMessageBox::Yes | QMessageBox::Cancel,
                    QMessageBox::Cancel);
                if (answer != QMessageBox::Yes)
                    return;
            }

            if (result.pairs.isEmpty())
            {
                QMessageBox::information(this, "Import CSV",
                    "No key-value pairs found in the file.");
                return;
            }

            // Let the user choose the target lexicon.
            // Build a list: discovered file display names + "New lexicon…"
            QStringList targetLabels;
            QStringList targetPaths;
            for (const QString& path : m_discoveredFiles)
            {
                targetLabels << m_fileAssetIds.value(path, QFileInfo(path).baseName());
                targetPaths  << path;
            }
            targetLabels << QStringLiteral("New lexicon\u2026");
            targetPaths  << QString{};

            // Pre-select the entry whose file name matches the suggestion from Row 1
            int defaultIdx = 0;
            if (!result.suggestedFilePath.isEmpty())
            {
                for (int i = 0; i < targetPaths.size(); ++i)
                {
                    if (QFileInfo(targetPaths[i]).fileName().compare(
                            result.suggestedFilePath, Qt::CaseInsensitive) == 0)
                    {
                        defaultIdx = i;
                        break;
                    }
                }
            }

            bool ok = false;
            const QString chosen = QInputDialog::getItem(
                this,
                "Import CSV — Target Lexicon",
                QString("Choose the lexicon to import %1 key(s) into:\n"
                        "(File suggests: '%2')")
                    .arg(result.pairs.size())
                    .arg(result.suggestedFilePath.isEmpty()
                         ? QStringLiteral("none") : result.suggestedFilePath),
                targetLabels,
                defaultIdx,
                /*editable=*/false,
                &ok);

            if (!ok)
                return;

            const int chosenIdx = targetLabels.indexOf(chosen);
            QString targetPath  = (chosenIdx >= 0) ? targetPaths[chosenIdx] : QString{};

            // "New lexicon…" chosen — create it inside the project source tree
            if (targetPath.isEmpty())
            {
                // Resolve the project source root so the dialog always opens inside
                // the project, never at the engine install path (O3DE's CWD).
                QString saveRoot = m_localisationDir;
                if (saveRoot.isEmpty())
                {
                    AZStd::string sourcePath;
                    LexiconEditorRequestBus::BroadcastResult(
                        sourcePath, &LexiconEditorRequests::GetProjectSourcePath);
                    if (!sourcePath.empty())
                    {
                        saveRoot = QString::fromUtf8(sourcePath.c_str());
                        if (!saveRoot.endsWith(QLatin1Char('/')))
                            saveRoot += QLatin1Char('/');
                        saveRoot += QLatin1String("Assets/Localisation");
                        QDir{}.mkpath(saveRoot); // create if not yet present
                    }
                }

                QString suggestedName = result.suggestedFilePath;
                if (!suggestedName.endsWith(QLatin1String(".helex"), Qt::CaseInsensitive))
                    suggestedName += QLatin1String(".helex");

                // Resolve the project source root for the out-of-tree guard below
                AZStd::string rawSourcePath;
                LexiconEditorRequestBus::BroadcastResult(
                    rawSourcePath, &LexiconEditorRequests::GetProjectSourcePath);
                const QString projectRoot = QDir(QString::fromUtf8(rawSourcePath.c_str()))
                                                .canonicalPath();

                // Loop until the user picks a valid path or cancels
                while (true)
                {
                    targetPath = QFileDialog::getSaveFileName(
                        this,
                        "New Lexicon for Import",
                        saveRoot.isEmpty()
                            ? suggestedName
                            : saveRoot + QLatin1Char('/') + suggestedName,
                        "HeLex Files (*.helex)");

                    if (targetPath.isEmpty())
                        return;

                    if (!targetPath.endsWith(QLatin1String(".helex"), Qt::CaseInsensitive))
                        targetPath += QLatin1String(".helex");

                    // Validate the chosen path is inside the project source tree
                    if (!projectRoot.isEmpty())
                    {
                        const QString canonical = QFileInfo(QFileInfo(targetPath).absolutePath())
                                                      .canonicalFilePath();
                        if (!canonical.startsWith(projectRoot))
                        {
                            QMessageBox::warning(
                                this,
                                "Invalid Save Location",
                                QString("Lexicon files must be saved inside the project folder:\n%1\n\n"
                                        "Please choose a location inside the project.")
                                    .arg(projectRoot));
                            saveRoot = projectRoot; // re-anchor the dialog for next attempt
                            targetPath.clear();
                            continue;
                        }
                    }
                    break;
                }

                const QString baseName = QFileInfo(targetPath).completeBaseName();
                QJsonObject newRoot;
                newRoot[QLatin1String("assetId")]  = baseName;
                newRoot[QLatin1String("cultures")] = QJsonArray{};
                newRoot[QLatin1String("entries")]  = QJsonObject{};

                QFile nf(targetPath);
                if (!nf.open(QIODevice::WriteOnly | QIODevice::Truncate))
                {
                    QMessageBox::warning(this, "Import CSV",
                        QString("Could not create file:\n%1").arg(targetPath));
                    return;
                }
                nf.write(QJsonDocument(newRoot).toJson(QJsonDocument::Indented));
                nf.close();
            }

            // Conflict check — count how many keys already have a non-empty value
            int conflicts = 0;
            {
                QFile cf(targetPath);
                if (cf.open(QIODevice::ReadOnly))
                {
                    QJsonParseError err;
                    const QJsonDocument doc = QJsonDocument::fromJson(cf.readAll(), &err);
                    cf.close();
                    if (err.error == QJsonParseError::NoError && doc.isObject())
                    {
                        const QJsonObject existing =
                            doc.object().value(QLatin1String("entries")).toObject();
                        for (auto it = result.pairs.cbegin(); it != result.pairs.cend(); ++it)
                        {
                            const QJsonValue ev = existing.value(it.key());
                            if (!ev.isUndefined() && !ev.toString().isEmpty())
                                ++conflicts;
                        }
                    }
                }
            }

            if (conflicts > 0)
            {
                const auto answer = QMessageBox::question(
                    this,
                    "Import CSV — Overwrite?",
                    QString("%1 key(s) already have a non-empty value in the target lexicon.\n"
                            "Do you want to overwrite them?")
                        .arg(conflicts),
                    QMessageBox::Yes | QMessageBox::Cancel,
                    QMessageBox::Cancel);
                if (answer != QMessageBox::Yes)
                    return;
            }

            // Write all pairs
            for (auto it = result.pairs.cbegin(); it != result.pairs.cend(); ++it)
                WriteValueToFile(targetPath, it.key(), QJsonValue(it.value()));

            // Rescan if a new file was created, otherwise sync + reload
            const bool isNew = !m_discoveredFiles.contains(targetPath);
            if (isNew)
            {
                LexiconEditorRequestBus::Broadcast(&LexiconEditorRequests::RefreshKeyTree);
                DiscoverFiles();
            }
            else
            {
                RunSuperSetSync();
                ReloadModels();
            }

            QMessageBox::information(this, "Import CSV",
                QString("Imported %1 key(s) into:\n%2")
                    .arg(result.pairs.size())
                    .arg(targetPath));
        }
    }

    void LexiconToolWindow::OnPreviewToggled(bool checked)
    {
        if (checked)
        {
            // Save the culture that is active right now so we can restore it on toggle-off.
            AZStd::string currentCulture;
            FoundationLocalisationRequestBus::BroadcastResult(
                currentCulture,
                &FoundationLocalisationRequests::GetActiveCulture);
            m_previewPriorCulture = QString::fromUtf8(currentCulture.c_str());

            // A valid Working file is required.
            const QString workPath = m_workingCombo->currentData().toString();
            if (workPath.isEmpty() || workPath == NewLexiconSentinel)
            {
                m_previewBtn->blockSignals(true);
                m_previewBtn->setChecked(false);
                m_previewBtn->blockSignals(false);
                QMessageBox::warning(this, "Live Preview",
                    "Select a Working lexicon before enabling preview.");
                return;
            }

            // Read the cultures array from the Working .helex.
            QStringList cultures;
            {
                QFile f(workPath);
                if (f.open(QIODevice::ReadOnly))
                {
                    QJsonParseError err;
                    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
                    f.close();
                    if (err.error == QJsonParseError::NoError && doc.isObject())
                    {
                        const QJsonArray arr =
                            doc.object().value(QLatin1String("cultures")).toArray();
                        for (const QJsonValue& v : arr)
                        {
                            const QString code = v.toString().trimmed();
                            if (!code.isEmpty())
                                cultures << code;
                        }
                    }
                }
            }

            if (cultures.isEmpty())
            {
                m_previewBtn->blockSignals(true);
                m_previewBtn->setChecked(false);
                m_previewBtn->blockSignals(false);
                QMessageBox::warning(this, "Live Preview",
                    "The Working lexicon has no cultures defined.\n\n"
                    "Add at least one culture code (e.g. 'en-GB') to its cultures list,\n"
                    "then try again.");
                return;
            }

            // Check the .lexicon product is in the asset catalog.
            // Derive the relative product path: source relative path with .lexicon extension.
            bool hbinReady = false;
            {
                AZStd::string srcRoot;
                LexiconEditorRequestBus::BroadcastResult(
                    srcRoot, &LexiconEditorRequests::GetProjectSourcePath);
                const QString projectDir = QString::fromUtf8(srcRoot.c_str());

                if (!projectDir.isEmpty())
                {
                    QString relPath = QDir(projectDir).relativeFilePath(workPath);
                    if (relPath.endsWith(QLatin1String(".helex"), Qt::CaseInsensitive))
                    {
                        relPath.chop(6); // remove ".helex"
                        relPath += QLatin1String(".lexicon");
                    }
                    // Asset catalog paths use lowercase forward slashes.
                    const QByteArray catalogPath =
                        relPath.toLower().replace(QLatin1Char('\\'), QLatin1Char('/')).toUtf8();

                    AZ::Data::AssetId assetId;
                    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
                        assetId,
                        &AZ::Data::AssetCatalogRequests::GetAssetIdByPath,
                        catalogPath.constData(),
                        azrtti_typeid<Heathen::LexiconAssemblyAsset>(),
                        false);
                    hbinReady = assetId.IsValid();
                }
            }

            if (!hbinReady)
            {
                const auto answer = QMessageBox::question(
                    this,
                    "Live Preview — Asset Not Ready",
                    "The .lexicon product for this lexicon has not been processed yet.\n\n"
                    "Run the Asset Processor to build the .lexicon first.\n\n"
                    "Enable preview anyway? (resolution calls will return empty until "
                    "the asset is loaded.)",
                    QMessageBox::Yes | QMessageBox::Cancel,
                    QMessageBox::Cancel);

                if (answer != QMessageBox::Yes)
                {
                    m_previewBtn->blockSignals(true);
                    m_previewBtn->setChecked(false);
                    m_previewBtn->blockSignals(false);
                    return;
                }
            }

            // Hot-swap the active culture to the first one declared in this lexicon.
            FoundationLocalisationRequestBus::Broadcast(
                &FoundationLocalisationRequests::LoadCulture,
                AZStd::string(cultures.first().toUtf8().constData()));

            m_previewBtn->setText(
                QString("\u25CF Preview: %1").arg(cultures.first()));
        }
        else
        {
            // Restore whatever was active before we switched on.
            FoundationLocalisationRequestBus::Broadcast(
                &FoundationLocalisationRequests::LoadCulture,
                AZStd::string(m_previewPriorCulture.toUtf8().constData()));
            m_previewPriorCulture.clear();
            m_previewBtn->setText(QStringLiteral("\u25CE Preview"));
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // Explorer slots

    void LexiconToolWindow::OnTreeItemSelectionChanged(QTreeWidgetItem* current,
                                                        QTreeWidgetItem* /*previous*/)
    {
        const QString path = current ? current->data(0, Qt::UserRole).toString() : QString{};

        // Pre-fill Add Key field with selected prefix
        m_keyWarningLabel->hide();
        if (!path.isEmpty())
            m_keyField->setText(path + QLatin1Char('.'));
        else
            m_keyField->clear();

        if (path.isEmpty())
        {
            m_inspector->ShowEmpty();
            return;
        }

        // Scroll table to matching row
        if (!m_syncingSelection)
        {
            m_syncingSelection = true;
            const int rowCount = m_tableModel->rowCount();
            for (int r = 0; r < rowCount; ++r)
            {
                const QModelIndex idx = m_tableModel->index(r, LexiconTableModel::ColKey);
                if (m_tableModel->data(idx, Qt::DisplayRole).toString() == path)
                {
                    m_tableView->selectionModel()->setCurrentIndex(
                        idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                    m_tableView->scrollTo(idx, QAbstractItemView::PositionAtCenter);
                    break;
                }
            }
            m_syncingSelection = false;
        }

        ShowInspectorForKey(path);
    }

    void LexiconToolWindow::OnTableSelectionChanged(const QModelIndex& current,
                                                     const QModelIndex& /*previous*/)
    {
        if (!current.isValid() || m_syncingSelection)
            return;

        const QString path = m_tableModel->data(
            m_tableModel->index(current.row(), LexiconTableModel::ColKey),
            Qt::DisplayRole).toString();

        if (path.isEmpty())
            return;

        // Update inspector
        ShowInspectorForKey(path);

        // Scroll tree to matching item
        m_syncingSelection = true;
        SelectTreeKey(path);
        m_syncingSelection = false;
    }

    void LexiconToolWindow::OnFileChanged(const QString& /*path*/)
    {
        // External file change — reload with a slight delay so any in-progress
        // write completes before we re-scan.
        QTimer::singleShot(300, this, [this]() { ReloadModels(); });
    }

    void LexiconToolWindow::ShowInspectorForKey(const QString& path)
    {
        const LexiconEntryMap& entryMap = m_treeModel->EntryMap();

        if (entryMap.contains(path))
        {
            const LexiconEntry entry = entryMap.value(path);

            if (entry.isAsset)
            {
                const QString uuid = entry.activeValue.isEmpty()
                                     ? entry.sourceValue
                                     : entry.activeValue;
                const QString resolvedPath = ResolveAssetPath(uuid);

                QString absoluteSrcPath;
                if (!resolvedPath.isEmpty())
                {
                    AZStd::string srcRoot;
                    LexiconEditorRequestBus::BroadcastResult(
                        srcRoot, &LexiconEditorRequests::GetProjectSourcePath);
                    if (!srcRoot.empty())
                    {
                        absoluteSrcPath = QString::fromUtf8(srcRoot.c_str());
                        if (!absoluteSrcPath.endsWith(QLatin1Char('/')))
                            absoluteSrcPath += QLatin1Char('/');
                        absoluteSrcPath += resolvedPath;
                    }
                }

                m_inspector->ShowAssetEntry(path, uuid, resolvedPath, absoluteSrcPath);
            }
            else
            {
                m_inspector->ShowStringEntry(path, entry.sourceValue, entry.activeValue);
            }
        }
        else
        {
            // Namespace node
            const QString nsPrefix = path + QLatin1Char('.');
            int keyCount = 0;
            for (auto it = entryMap.cbegin(); it != entryMap.cend(); ++it)
            {
                const QString& key = it.key();
                if (key == path || key.startsWith(nsPrefix))
                    ++keyCount;
            }
            m_inspector->ShowNamespace(path, keyCount);
        }
    }

    void LexiconToolWindow::RebuildTagTree()
    {
        // Save selection before clearing
        const QString savedKey = GetSelectedTreeKey();

        // Block signals while rebuilding to avoid spurious inspector updates
        m_treeWidget->blockSignals(true);
        m_treeWidget->clear();
        m_treeWidget->blockSignals(false);

        // Update file watcher paths
        if (!m_fileWatcher->files().isEmpty())
            m_fileWatcher->removePaths(m_fileWatcher->files());
        if (!m_discoveredFiles.isEmpty())
            m_fileWatcher->addPaths(m_discoveredFiles);

        const LexiconEntryMap& entryMap = m_treeModel->EntryMap();
        if (entryMap.isEmpty())
            return;

        // Build segment tree
        QMap<QString, QTreeWidgetItem*> itemsByPath;
        QStringList sortedKeys = entryMap.keys();
        sortedKeys.sort();

        for (const QString& key : sortedKeys)
        {
            const QStringList segments = key.split(QLatin1Char('.'));
            QString currentPath;
            QTreeWidgetItem* parent = nullptr;

            for (int i = 0; i < segments.size(); ++i)
            {
                currentPath = currentPath.isEmpty()
                              ? segments[i]
                              : currentPath + QLatin1Char('.') + segments[i];

                if (!itemsByPath.contains(currentPath))
                {
                    QTreeWidgetItem* item = parent
                        ? new QTreeWidgetItem(parent)
                        : new QTreeWidgetItem(m_treeWidget);
                    item->setText(0, segments[i]);
                    item->setData(0, Qt::UserRole, currentPath);
                    itemsByPath[currentPath] = item;
                }
                parent = itemsByPath[currentPath];
            }
        }

        // Add button widgets to leaf nodes only (namespace nodes get nothing)
        for (auto it = itemsByPath.begin(); it != itemsByPath.end(); ++it)
        {
            const QString&  path    = it.key();
            QTreeWidgetItem* item   = it.value();
            const bool isLeaf       = entryMap.contains(path);
            const bool hasParent    = path.contains(QLatin1Char('.'));
            QWidget* btn = MakeNodeButtons(path, isLeaf, hasParent);
            if (btn)
                m_treeWidget->setItemWidget(item, 1, btn);
        }

        m_treeWidget->expandAll();

        // Restore selection (fires OnTreeItemSelectionChanged)
        if (!savedKey.isEmpty())
            SelectTreeKey(savedKey);
    }

    void LexiconToolWindow::SelectTreeKey(const QString& path)
    {
        if (path.isEmpty())
            return;

        QTreeWidgetItemIterator it(m_treeWidget);
        while (*it)
        {
            if ((*it)->data(0, Qt::UserRole).toString() == path)
            {
                m_treeWidget->setCurrentItem(*it);
                m_treeWidget->scrollToItem(*it, QAbstractItemView::PositionAtCenter);
                return;
            }
            ++it;
        }
    }

    QString LexiconToolWindow::GetSelectedTreeKey() const
    {
        QTreeWidgetItem* item = m_treeWidget ? m_treeWidget->currentItem() : nullptr;
        return item ? item->data(0, Qt::UserRole).toString() : QString{};
    }

    QWidget* LexiconToolWindow::MakeNodeButtons(const QString& fullPath,
                                                 bool isLeaf, bool /*hasParent*/)
    {
        // Only leaf nodes get a button — the top-bar Add Key field handles all insertions.
        if (!isLeaf)
            return nullptr;

        auto* w  = new QWidget;
        auto* hl = new QHBoxLayout(w);
        hl->setContentsMargins(2, 1, 2, 1);
        hl->setSpacing(2);
        hl->addStretch();

        auto* removeBtn = new QPushButton("X", w);
        removeBtn->setFixedSize(22, 20);
        removeBtn->setStyleSheet("color: #cc3333; font-weight: bold;");
        removeBtn->setToolTip("Remove this key from all lexicons");
        connect(removeBtn, &QPushButton::clicked, this, [this, fullPath]()
        {
            const auto answer = QMessageBox::question(
                this, "Remove Key",
                QString("Remove key \"%1\" from all %2 discovered file(s)?\n\nThis cannot be undone.")
                    .arg(fullPath)
                    .arg(m_discoveredFiles.size()),
                QMessageBox::Yes | QMessageBox::Cancel,
                QMessageBox::Cancel);

            if (answer != QMessageBox::Yes)
                return;

            RemoveKeyFromAllFiles(fullPath);
            RunSuperSetSync();
            ReloadModels();
        });
        hl->addWidget(removeBtn);

        return w;
    }

    void LexiconToolWindow::OnAddKey()
    {
        m_keyWarningLabel->hide();

        if (m_discoveredFiles.isEmpty())
        {
            m_keyWarningLabel->setText(
                "\u26A0 No .helex files found. Open the Working dropdown and choose New\u2026");
            m_keyWarningLabel->show();
            return;
        }

        const QString key = m_keyField->text().trimmed();

        // Validate syntax
        if (key.isEmpty() || key.startsWith(QLatin1Char('.')) ||
            key.endsWith(QLatin1Char('.')) || key.contains(QLatin1String("..")))
        {
            m_keyWarningLabel->setText("Invalid key — check for leading, trailing, or double dots.");
            m_keyWarningLabel->show();
            return;
        }

        // Check uniqueness against the current super-set
        if (m_treeModel->EntryMap().contains(key))
        {
            m_keyWarningLabel->setText(QString("\u26A0 Key \"%1\" already exists.").arg(key));
            m_keyWarningLabel->show();
            return;
        }

        // Write to all discovered files then reload — always, no early return
        WriteKeyToAllFiles(key);
        RunSuperSetSync();
        ReloadModels();

        // Confirm the key appeared in the reloaded map
        if (!m_treeModel->EntryMap().contains(key))
        {
            m_keyWarningLabel->setText(
                QString("\u26A0 Key \"%1\" was not found after reload — write may have failed.").arg(key));
            m_keyWarningLabel->show();
        }

        // Re-apply prefix to key field for quick sibling entry
        const QString prefix = GetSelectedTreeKey();
        m_keyField->setText(prefix.isEmpty() ? QString{} : prefix + QLatin1Char('.'));
    }

    ////////////////////////////////////////////////////////////////////////
    // Workbench write-back

    void LexiconToolWindow::OnActiveValueEdited(const QString& key, const QString& newValue)
    {
        const QString workPath = m_workingCombo->currentData().toString();
        if (workPath.isEmpty())
            return;

        if (!WriteValueToFile(workPath, key, QJsonValue(newValue)))
        {
            QMessageBox::warning(this, "Write Failed",
                QString("Could not write to working file:\n%1").arg(workPath));
            return;
        }

        // Reload so the table and tree reflect the written value
        ReloadModels();
    }

    void LexiconToolWindow::OnInspectorValueEdited(const QString& key, const QString& value)
    {
        const QString workPath = m_workingCombo->currentData().toString();
        if (workPath.isEmpty() || workPath == NewLexiconSentinel)
            return;

        if (!WriteValueToFile(workPath, key, QJsonValue(value)))
        {
            QMessageBox::warning(this, "Write Failed",
                QString("Could not write to working file:\n%1").arg(workPath));
            return;
        }

        // Reload so the inspector shows the freshly-persisted value on next selection.
        // RebuildTagTree() restores the tree selection so the inspector re-populates.
        ReloadModels();
    }

    void LexiconToolWindow::OnInspectorBrowseAsset(const QString& key)
    {
        // Full O3DE asset browser integration is planned for Phase 6.8.
        // For now, accept a manually-entered UUID so the workflow is functional.
        bool ok = false;
        const QString uuid = QInputDialog::getText(
            this,
            "Set Asset Reference",
            "Paste the asset UUID (e.g. {AABBCCDD-...}):",
            QLineEdit::Normal,
            QString{},
            &ok);

        if (!ok || uuid.trimmed().isEmpty())
            return;

        const QString workPath = m_workingCombo->currentData().toString();
        if (workPath.isEmpty() || workPath == NewLexiconSentinel)
            return;

        // Write as asset object format: {"uuid": "..."}
        QJsonObject obj;
        obj[QLatin1String("uuid")] = uuid.trimmed();
        if (!WriteValueToFile(workPath, key, QJsonValue(obj)))
        {
            QMessageBox::warning(this, "Write Failed",
                QString("Could not write to working file:\n%1").arg(workPath));
            return;
        }

        // Refresh so the inspector and table both show the new asset entry
        RunSuperSetSync();
        ReloadModels();
    }

    void LexiconToolWindow::OnGathererItemsAccepted([[maybe_unused]] const QStringList& keys)
    {
        // Items were written to the working .helex — sync all files then reload.
        RunSuperSetSync();
        ReloadModels();
    }

    ////////////////////////////////////////////////////////////////////////
    // Private — file operations

    void LexiconToolWindow::DiscoverFiles()
    {
        // Save current selections so we can restore them after rebuilding dropdowns
        const QString curRef  = m_referenceCombo->currentData().toString();
        const QString curWork = m_workingCombo->currentData().toString();

        // Track the previous set so we can detect newly-added files for cross-pollination.
        // An empty previousFiles means this is the first discovery — skip cross-pollination
        // so we don't fire it on every tool open.
        const QStringList previousFiles = m_discoveredFiles;

        m_discoveredFiles.clear();
        m_fileAssetIds.clear();

        // Get the project source path the EditorSystemComponent already verified
        AZStd::string sourcePath;
        LexiconEditorRequestBus::BroadcastResult(
            sourcePath, &LexiconEditorRequests::GetProjectSourcePath);
        const QString projectDir = QString::fromUtf8(sourcePath.c_str());

        // Scan with QDirIterator — guaranteed native absolute paths QFile can open
        auto scanForFiles = [this, &projectDir]()
        {
            if (projectDir.isEmpty())
                return;
            QDirIterator it(projectDir,
                            QStringList() << QStringLiteral("*.helex"),
                            QDir::Files,
                            QDirIterator::Subdirectories);
            while (it.hasNext())
                m_discoveredFiles.append(it.next());
        };

        scanForFiles();

        // Auto-create default.helex when the project has no lexicons yet
        if (m_discoveredFiles.isEmpty())
        {
            CreateDefaultLexicon();
            scanForFiles(); // re-populate after creation
        }

        // N: sort by filename so all dropdowns and displays are alphabetically consistent
        m_discoveredFiles.sort(Qt::CaseInsensitive);

        // Read each file's assetId for dropdown labels, and locate default.helex
        // to anchor the browse root for New lexicon dialogs.
        m_localisationDir.clear();
        for (const QString& path : m_discoveredFiles)
        {
            QString label = QFileInfo(path).baseName(); // fallback
            QFile file(path);
            if (file.open(QIODevice::ReadOnly))
            {
                QJsonParseError err;
                const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
                file.close();
                if (err.error == QJsonParseError::NoError && doc.isObject())
                {
                    const QString id = doc.object().value(QLatin1String("assetId")).toString().trimmed();
                    if (!id.isEmpty())
                        label = id;
                }
            }
            m_fileAssetIds[path] = label;

            // The directory of default.helex is the workspace root for new files
            if (m_localisationDir.isEmpty() &&
                QFileInfo(path).baseName().compare(QLatin1String("default"),
                    Qt::CaseInsensitive) == 0)
            {
                m_localisationDir = QFileInfo(path).absolutePath();
            }
        }

        // If no default.helex found, fall back to project's Assets/Localisation dir
        if (m_localisationDir.isEmpty() && !m_discoveredFiles.isEmpty())
            m_localisationDir = QFileInfo(m_discoveredFiles.first()).absolutePath();

        // Cross-pollination: detect files added since the last discovery and inject
        // Language.* keys across the workspace. Skipped on first open (previousFiles empty).
        if (!previousFiles.isEmpty())
        {
            QStringList newFiles;
            for (const QString& path : m_discoveredFiles)
                if (!previousFiles.contains(path))
                    newFiles.append(path);

            if (!newFiles.isEmpty())
                RunCrossPollination(newFiles);
        }

        RebuildDropdowns(curRef, curWork);

        // Run super-set sync silently on discovery
        RunSuperSetSync();
        ReloadModels();
    }

    void LexiconToolWindow::RunCrossPollination(const QStringList& newFiles)
    {
        // Build the set of Language.* injections needed across the workspace.
        // For each new file N (assetId A_N) and every other discovered file O (assetId A_O):
        //   - N needs Language.A_O (so it can name the other language in its culture)
        //   - O needs Language.A_N (so it can name the new language in its culture)
        // Using QMap<path, QSet<key>> deduplicates automatically.

        QMap<QString, QSet<QString>> toInject;

        for (const QString& newPath : newFiles)
        {
            const QString newAssetId =
                m_fileAssetIds.value(newPath, QFileInfo(newPath).baseName());

            for (const QString& otherPath : m_discoveredFiles)
            {
                if (otherPath == newPath)
                    continue;

                const QString otherAssetId =
                    m_fileAssetIds.value(otherPath, QFileInfo(otherPath).baseName());

                toInject[newPath].insert(QStringLiteral("Language.") + otherAssetId);
                toInject[otherPath].insert(QStringLiteral("Language.") + newAssetId);
            }
        }

        if (toInject.isEmpty())
            return;

        // Filter out keys that already exist in each file, building the confirmed pending set.
        QMap<QString, QStringList> pending; // filePath → sorted list of keys to add

        for (auto it = toInject.cbegin(); it != toInject.cend(); ++it)
        {
            QStringList missing;
            QFile f(it.key());
            if (!f.open(QIODevice::ReadOnly))
                continue;
            QJsonParseError err;
            const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
            f.close();
            if (err.error != QJsonParseError::NoError || !doc.isObject())
                continue;

            const QJsonObject entries =
                doc.object().value(QLatin1String("entries")).toObject();

            for (const QString& key : it.value())
                if (!entries.contains(key))
                    missing << key;

            missing.sort();
            if (!missing.isEmpty())
                pending.insert(it.key(), missing);
        }

        if (pending.isEmpty())
            return;

        // ---- Diff-style confirmation dialog ----
        QString diffText;
        for (auto it = pending.cbegin(); it != pending.cend(); ++it)
        {
            diffText += QString("In %1:\n").arg(QFileInfo(it.key()).fileName());
            for (const QString& key : it.value())
                diffText += QString("  + %1 = \"\"\n").arg(key);
            diffText += QLatin1Char('\n');
        }

        QDialog dlg(this);
        dlg.setWindowTitle("Cross-Pollination");
        auto* dlgLayout = new QVBoxLayout(&dlg);

        auto* infoLabel = new QLabel(
            QString("New lexicon file(s) detected. The following Language.* keys will be\n"
                    "injected to keep all files consistent:"),
            &dlg);
        dlgLayout->addWidget(infoLabel);

        auto* diffView = new QTextEdit(&dlg);
        diffView->setReadOnly(true);
        diffView->setPlainText(diffText);
        diffView->setMinimumSize(480, 220);
        diffView->setFont(QFont(QStringLiteral("Monospace"), 9));
        dlgLayout->addWidget(diffView);

        auto* dlgButtons = new QDialogButtonBox(&dlg);
        auto* applyBtn = dlgButtons->addButton("Apply", QDialogButtonBox::AcceptRole);
        auto* skipBtn  = dlgButtons->addButton("Skip",  QDialogButtonBox::RejectRole);
        Q_UNUSED(applyBtn);
        Q_UNUSED(skipBtn);
        connect(dlgButtons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(dlgButtons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        dlgLayout->addWidget(dlgButtons);

        if (dlg.exec() != QDialog::Accepted)
            return;

        // ---- Apply ----
        for (auto it = pending.cbegin(); it != pending.cend(); ++it)
            for (const QString& key : it.value())
                WriteValueToFile(it.key(), key, QJsonValue(QString{}));
    }

    void LexiconToolWindow::RebuildDropdowns(const QString& keepReference,
                                              const QString& keepWorking)
    {
        // ---- Reference combo ----
        // Always: "——" (no reference) + all discovered files except the working one
        m_referenceCombo->blockSignals(true);
        m_referenceCombo->clear();
        m_referenceCombo->addItem(QStringLiteral("\u2014\u2014"), NoReference); // "——"
        for (const QString& path : m_discoveredFiles)
        {
            if (path == keepWorking)
                continue; // exclude whatever Working has
            const QString label = m_fileAssetIds.value(path, QFileInfo(path).baseName());
            m_referenceCombo->addItem(label, path);
            m_referenceCombo->setItemData(m_referenceCombo->count() - 1, path, Qt::ToolTipRole);
        }
        {
            int idx = m_referenceCombo->findData(keepReference);
            m_referenceCombo->setCurrentIndex(idx >= 0 ? idx : 0); // default to "——"
        }
        m_referenceCombo->blockSignals(false);

        // ---- Working combo ----
        // All discovered files except the reference one + "New…" sentinel at end
        m_workingCombo->blockSignals(true);
        m_workingCombo->clear();
        for (const QString& path : m_discoveredFiles)
        {
            if (!keepReference.isEmpty() && path == keepReference)
                continue; // exclude whatever Reference has (not if Reference is "——")
            const QString label = m_fileAssetIds.value(path, QFileInfo(path).baseName());
            m_workingCombo->addItem(label, path);
            m_workingCombo->setItemData(m_workingCombo->count() - 1, path, Qt::ToolTipRole);
        }
        m_workingCombo->addItem(QStringLiteral("New\u2026"), NewLexiconSentinel);
        {
            int idx = m_workingCombo->findData(keepWorking);
            m_workingCombo->setCurrentIndex(idx >= 0 ? idx : 0);
        }
        m_workingCombo->blockSignals(false);

        // Update Add Key bar enabled state
        const bool hasWork = !m_workingCombo->currentData().toString().isEmpty()
                           && m_workingCombo->currentData().toString() != NewLexiconSentinel;
        m_addKeyBtn->setEnabled(hasWork);
        m_keyField->setEnabled(hasWork);

        UpdateReferenceColumnVisibility();
    }

    int LexiconToolWindow::RunSuperSetSync()
    {
        if (m_discoveredFiles.isEmpty())
            return 0;

        // Build super-set of all keys across all files
        QSet<QString> allKeys;
        for (const QString& path : m_discoveredFiles)
        {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly))
                continue;
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
            file.close();
            if (err.error != QJsonParseError::NoError || !doc.isObject())
                continue;
            const QJsonObject entries = doc.object().value(QLatin1String("entries")).toObject();
            for (auto it = entries.begin(); it != entries.end(); ++it)
                allKeys.insert(it.key());
        }

        // Ensure Language.<assetId> exists in the super-set for every discovered file.
        // This guarantees every lexicon always has a self-name entry and entries naming
        // every peer language, regardless of whether the explicit injection ran.
        for (const QString& path : m_discoveredFiles)
        {
            QFile idFile(path);
            if (!idFile.open(QIODevice::ReadOnly))
                continue;
            QJsonParseError idErr;
            QJsonDocument idDoc = QJsonDocument::fromJson(idFile.readAll(), &idErr);
            idFile.close();
            if (idErr.error != QJsonParseError::NoError || !idDoc.isObject())
                continue;
            const QString assetId =
                idDoc.object().value(QLatin1String("assetId")).toString().trimmed();
            if (!assetId.isEmpty())
                allKeys.insert(QStringLiteral("Language.") + assetId);
        }

        if (allKeys.isEmpty())
            return 0;

        // Patch files that are missing keys
        int patchedFiles = 0;
        for (const QString& path : m_discoveredFiles)
        {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly))
                continue;
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
            file.close();
            if (err.error != QJsonParseError::NoError || !doc.isObject())
                continue;

            QJsonObject root    = doc.object();
            QJsonObject entries = root.value(QLatin1String("entries")).toObject();

            bool modified = false;
            for (const QString& key : allKeys)
            {
                if (!entries.contains(key))
                {
                    entries[key] = QString{};
                    modified = true;
                }
            }

            if (modified)
            {
                root[QLatin1String("entries")] = entries;
                doc.setObject(root);
                if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                {
                    file.write(doc.toJson(QJsonDocument::Indented));
                    file.close();
                    ++patchedFiles;
                }
            }
        }

        return patchedFiles;
    }

    void LexiconToolWindow::WriteKeyToAllFiles(const QString& key, const QString& value)
    {
        for (const QString& path : m_discoveredFiles)
        {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly))
                continue;
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
            file.close();
            if (err.error != QJsonParseError::NoError || !doc.isObject())
                continue;

            QJsonObject root    = doc.object();
            QJsonObject entries = root.value(QLatin1String("entries")).toObject();

            if (entries.contains(key))
                continue; // already present

            entries[key] = value;
            root[QLatin1String("entries")] = entries;
            doc.setObject(root);

            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                file.write(doc.toJson(QJsonDocument::Indented));
                file.close();
            }
        }
    }

    void LexiconToolWindow::RemoveKeyFromAllFiles(const QString& key)
    {
        for (const QString& path : m_discoveredFiles)
        {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly))
                continue;
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
            file.close();
            if (err.error != QJsonParseError::NoError || !doc.isObject())
                continue;

            QJsonObject root    = doc.object();
            QJsonObject entries = root.value(QLatin1String("entries")).toObject();

            if (!entries.contains(key))
                continue;

            entries.remove(key);
            root[QLatin1String("entries")] = entries;
            doc.setObject(root);

            if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                file.write(doc.toJson(QJsonDocument::Indented));
                file.close();
            }
        }
    }

    void LexiconToolWindow::RemoveKeysFromAllFiles(const QStringList& keys)
    {
        if (keys.isEmpty())
            return;

        for (const QString& path : m_discoveredFiles)
        {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly))
                continue;
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
            file.close();
            if (err.error != QJsonParseError::NoError || !doc.isObject())
                continue;

            QJsonObject root    = doc.object();
            QJsonObject entries = root.value(QLatin1String("entries")).toObject();

            bool modified = false;
            for (const QString& key : keys)
            {
                if (entries.contains(key))
                {
                    entries.remove(key);
                    modified = true;
                }
            }

            if (modified)
            {
                root[QLatin1String("entries")] = entries;
                doc.setObject(root);
                if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                {
                    file.write(doc.toJson(QJsonDocument::Indented));
                    file.close();
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // Context menu — Remove Key / Remove Namespace

    void LexiconToolWindow::OnContextMenuRequested(const QPoint& pos)
    {
        QTreeWidgetItem* item = m_treeWidget->itemAt(pos);
        if (!item)
            return;

        const QString fullPath = item->data(0, Qt::UserRole).toString();
        if (fullPath.isEmpty())
            return;

        const LexiconEntryMap& entryMap = m_treeModel->EntryMap();
        const bool isLeaf      = entryMap.contains(fullPath);
        const bool hasChildren = (item->childCount() > 0);

        QMenu menu(this);

        // "Remove Key" — removes only this exact key's entry, not its children.
        if (isLeaf)
        {
            QAction* removeKeyAction = menu.addAction(
                QString("Remove Key \"%1\"\u2026").arg(fullPath));
            connect(removeKeyAction, &QAction::triggered, this, [this, fullPath]()
            {
                const int fileCount = m_discoveredFiles.size();
                const auto answer = QMessageBox::question(
                    this,
                    "Remove Key",
                    QString("Remove key \"%1\" from all %2 discovered file(s)?\n\n"
                            "This cannot be undone.")
                        .arg(fullPath)
                        .arg(fileCount),
                    QMessageBox::Yes | QMessageBox::Cancel,
                    QMessageBox::Cancel);

                if (answer != QMessageBox::Yes)
                    return;

                RemoveKeyFromAllFiles(fullPath);
                RunSuperSetSync();
                ReloadModels();
            });
        }

        // "Remove Namespace" — removes this key AND all keys under this prefix.
        if (hasChildren)
        {
            const QString nsPrefix = fullPath + QLatin1Char('.');
            QStringList namespaceKeys;
            for (auto it = entryMap.begin(); it != entryMap.end(); ++it)
            {
                const QString& key = it.key();
                if (key == fullPath || key.startsWith(nsPrefix))
                    namespaceKeys.append(key);
            }

            if (!namespaceKeys.isEmpty())
            {
                QAction* removeNsAction = menu.addAction(
                    QString("Remove Namespace \"%1\"\u2026").arg(fullPath));
                connect(removeNsAction, &QAction::triggered, this, [this, fullPath, namespaceKeys]()
                {
                    const int fileCount = m_discoveredFiles.size();
                    const auto answer = QMessageBox::question(
                        this,
                        "Remove Namespace",
                        QString("Remove all %1 key(s) under \"%2\" from all %3 discovered file(s)?\n\n"
                                "This cannot be undone.")
                            .arg(namespaceKeys.size())
                            .arg(fullPath)
                            .arg(fileCount),
                        QMessageBox::Yes | QMessageBox::Cancel,
                        QMessageBox::Cancel);

                    if (answer != QMessageBox::Yes)
                        return;

                    RemoveKeysFromAllFiles(namespaceKeys);
                    RunSuperSetSync();
                    ReloadModels();
                });
            }
        }

        if (menu.isEmpty())
            return;

        menu.exec(m_treeWidget->viewport()->mapToGlobal(pos));
    }

    void LexiconToolWindow::CreateDefaultLexicon()
    {
        // Use the path the EditorSystemComponent already verified works for scanning
        AZStd::string sourcePath;
        LexiconEditorRequestBus::BroadcastResult(
            sourcePath, &LexiconEditorRequests::GetProjectSourcePath);

        if (sourcePath.empty())
        {
            QMessageBox::warning(this, "Cannot Create Lexicon",
                "Project source path is not available.\n"
                "Please use 'New…' in the Working dropdown to create your first lexicon manually.");
            return;
        }

        QString dir = QString::fromUtf8(sourcePath.c_str());
        if (!dir.endsWith(QLatin1Char('/')))
            dir += QLatin1Char('/');
        dir += QLatin1String("Assets/Localisation");

        // Create Assets/Localisation/ if it doesn't exist
        if (!QDir{dir}.exists() && !QDir{}.mkpath(dir))
        {
            QMessageBox::warning(this, "Cannot Create Lexicon",
                QString("Could not create directory:\n%1").arg(dir));
            return;
        }

        const QString defaultPath = dir + QLatin1String("/default.helex");

        QJsonObject root;
        root[QLatin1String("assetId")]  = QStringLiteral("default");
        root[QLatin1String("cultures")] = QJsonArray{};
        root[QLatin1String("entries")]  = QJsonObject{};

        QFile file(defaultPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            QMessageBox::warning(this, "Cannot Create Lexicon",
                QString("Could not write default.helex to:\n%1\n\n"
                        "Use 'New…' in the Working dropdown to choose a location manually.")
                    .arg(defaultPath));
            return;
        }
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();

        QMessageBox::information(this, "Lexicon Created",
            QString("Created default.helex at:\n%1").arg(defaultPath));

        // Ask the system component to rescan so GetKnownFilePaths() returns the new file
        LexiconEditorRequestBus::Broadcast(&LexiconEditorRequests::RefreshKeyTree);
    }

    bool LexiconToolWindow::WriteValueToFile(const QString& filePath,
                                              const QString& key,
                                              const QJsonValue& value)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
            return false;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
        file.close();

        if (err.error != QJsonParseError::NoError || !doc.isObject())
            return false;

        QJsonObject root    = doc.object();
        QJsonObject entries = root.value(QLatin1String("entries")).toObject();
        entries[key] = value;
        root[QLatin1String("entries")] = entries;
        doc.setObject(root);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
            return false;

        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        return true;
    }

    void LexiconToolWindow::ReloadModels()
    {
        const QString refPath  = m_referenceCombo->currentData().toString();
        const QString workPath = m_workingCombo->currentData().toString();
        const QString safeWork = (workPath == NewLexiconSentinel) ? QString{} : workPath;

        const bool hasSource = !refPath.isEmpty();
        const bool hasActive = !safeWork.isEmpty();

        // 1. Parse both files into the tree model
        m_treeModel->LoadFiles(refPath, safeWork);

        // 2. Run validation on a copy of the parsed entry map
        LexiconEntryMap validatedEntries = m_treeModel->EntryMap();
        const ValidationSummary summary  = LexiconValidator::Validate(validatedEntries,
                                                                       hasSource, hasActive);

        // 3. Push validated flags back into the tree so colours + badges update
        m_treeModel->ApplyValidationFlags(validatedEntries);

        // 4. Populate the workbench table — show ALL keys (no prefix filter)
        m_tableModel->SetEntries(validatedEntries, QString{}, hasSource, hasActive);

        // 5. Rebuild the visual tree (preserves and restores selection)
        RebuildTagTree();

        // 6. Update validation status bar
        if (!hasSource && !hasActive)
        {
            m_validationLabel->setText("No files open.");
            m_validationLabel->setStyleSheet("font-size: 11px; color: #a0a0a0;");
        }
        else if (!summary.HasIssues())
        {
            m_validationLabel->setText(
                QString("%1 keys — all translated.").arg(summary.total));
            m_validationLabel->setStyleSheet("font-size: 11px; color: #60c060;");
        }
        else
        {
            QStringList parts;
            if (summary.missing    > 0) parts << QString("%1 missing").arg(summary.missing);
            if (summary.orphans    > 0) parts << QString("%1 orphan(s)").arg(summary.orphans);
            if (summary.duplicates > 0) parts << QString("%1 duplicate(s)").arg(summary.duplicates);
            if (summary.empty      > 0) parts << QString("%1 empty").arg(summary.empty);
            m_validationLabel->setText(
                QString("%1 keys — %2.").arg(summary.total).arg(parts.join(QStringLiteral(" · "))));
            m_validationLabel->setStyleSheet("font-size: 11px; color: #c09020;");
        }

        // If nothing is selected after rebuild, clear the inspector
        if (GetSelectedTreeKey().isEmpty())
            m_inspector->ShowEmpty();
    }

    ////////////////////////////////////////////////////////////////////////
    // Private — layout helpers

    QWidget* LexiconToolWindow::MakeExplorerPanel()
    {
        auto* panel = new QWidget(m_horizontalSplitter);
        auto* layout = new QVBoxLayout(panel);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(2);

        // Add Key bar
        auto* barWidget = new QWidget(panel);
        auto* barLayout = new QHBoxLayout(barWidget);
        barLayout->setContentsMargins(2, 2, 2, 2);
        barLayout->setSpacing(4);

        m_keyField = new QLineEdit(barWidget);
        m_keyField->setPlaceholderText("e.g. Menu.Play");
        m_keyField->setToolTip(
            "Type a full dot-path key to add to all lexicons.\n"
            "Selecting a tree node pre-fills this field with its path.");
        m_keyField->setEnabled(false);
        connect(m_keyField, &QLineEdit::returnPressed, this, &LexiconToolWindow::OnAddKey);
        barLayout->addWidget(m_keyField, 1);

        m_addKeyBtn = new QPushButton("+", barWidget);
        m_addKeyBtn->setFixedWidth(28);
        m_addKeyBtn->setToolTip("Add this key to all discovered .helex files.");
        m_addKeyBtn->setEnabled(false);
        connect(m_addKeyBtn, &QPushButton::clicked, this, &LexiconToolWindow::OnAddKey);
        barLayout->addWidget(m_addKeyBtn);

        layout->addWidget(barWidget);

        // Warning label (hidden by default)
        m_keyWarningLabel = new QLabel(panel);
        m_keyWarningLabel->setStyleSheet("color: #e0a020; font-size: 10px;");
        m_keyWarningLabel->setWordWrap(true);
        m_keyWarningLabel->hide();
        layout->addWidget(m_keyWarningLabel);

        // Tree widget — 2 columns: name (stretch) + buttons (fixed)
        static constexpr int kBtnColWidth = 84;
        m_treeWidget = new QTreeWidget(panel);
        m_treeWidget->setColumnCount(2);
        m_treeWidget->setHeaderHidden(true);
        m_treeWidget->setUniformRowHeights(true);
        m_treeWidget->setAnimated(true);
        m_treeWidget->setRootIsDecorated(true);
        m_treeWidget->header()->setStretchLastSection(false);
        m_treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_treeWidget->header()->setSectionResizeMode(1, QHeaderView::Fixed);
        m_treeWidget->header()->resizeSection(1, kBtnColWidth);
        m_treeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
        m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_treeWidget, &QTreeWidget::customContextMenuRequested,
                this, &LexiconToolWindow::OnContextMenuRequested);
        layout->addWidget(m_treeWidget, 1);

        return panel;
    }

    QWidget* LexiconToolWindow::MakePlaceholder(const QString& name, const QString& note)
    {
        auto* frame = new QFrame(this);
        frame->setFrameShape(QFrame::StyledPanel);

        auto* layout = new QVBoxLayout(frame);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(4);

        auto* nameLabel = new QLabel(QString("<b>%1</b>").arg(name), frame);
        layout->addWidget(nameLabel);

        auto* noteLabel = new QLabel(note, frame);
        noteLabel->setStyleSheet("color: #666; font-size: 11px;");
        noteLabel->setWordWrap(true);
        layout->addWidget(noteLabel);

        layout->addStretch(1);
        return frame;
    }

    void LexiconToolWindow::UpdateReferenceColumnVisibility()
    {
        const bool hasRef = !m_referenceCombo->currentData().toString().isEmpty();
        m_tableView->horizontalHeader()->setSectionHidden(LexiconTableModel::ColSource, !hasRef);
    }

    ////////////////////////////////////////////////////////////////////////
    // Private — settings

    void LexiconToolWindow::SaveSplitterState()
    {
        QSettings settings;
        settings.beginGroup(SettingsGroup);
        settings.setValue(VSplitterKey, m_verticalSplitter->saveState());
        settings.setValue(HSplitterKey, m_horizontalSplitter->saveState());
        settings.endGroup();
    }

    void LexiconToolWindow::RestoreSplitterState()
    {
        QSettings settings;
        settings.beginGroup(SettingsGroup);

        const QByteArray vs = settings.value(VSplitterKey).toByteArray();
        if (!vs.isEmpty())
            m_verticalSplitter->restoreState(vs);

        const QByteArray hs = settings.value(HSplitterKey).toByteArray();
        if (!hs.isEmpty())
            m_horizontalSplitter->restoreState(hs);

        settings.endGroup();
    }

    ////////////////////////////////////////////////////////////////////////

} // namespace FoundationLocalisation

#include <moc_LexiconToolWindow.cpp>
