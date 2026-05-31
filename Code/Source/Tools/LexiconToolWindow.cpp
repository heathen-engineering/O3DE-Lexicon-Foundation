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
#include "KnownCultures.h"
#include "LexiconCsvInterop.h"
#include "LexiconEditorRequestBus.h"
#include "LexiconGathererInboxWidget.h"
#include "LexiconHintTypeDelegate.h"
#include "LexiconTableModel.h"
#include "LexiconTreeModel.h"
#include "LexiconValidator.h"
#include "LexiconValueDelegate.h"
#include "LexiconNewFileDialog.h"

#include <FoundationLocalisation/FoundationLocalisationBus.h>
#include <FoundationLocalisation/LexiconAssemblyAsset.h>

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFont>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <functional>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QSettings>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabBar>
#include <QTableView>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace FoundationLocalisation
{
    // ── Key column delegate ───────────────────────────────────────────────────
    // Renders [×] on the left of each key cell. Clicking [×] triggers delete.
    namespace
    {
        static constexpr int kDelW = 22; // width reserved for the × button

        class LexiconKeyDelegate : public QStyledItemDelegate
        {
        public:
            using DeleteFn = std::function<void(const QString&)>;

            explicit LexiconKeyDelegate(DeleteFn fn, QObject* parent = nullptr)
                : QStyledItemDelegate(parent), m_deleteFn(std::move(fn)) {}

            void paint(QPainter* p, const QStyleOptionViewItem& opt,
                       const QModelIndex& idx) const override
            {
                // Draw the standard item background (selection highlight etc.)
                QStyleOptionViewItem o = opt;
                initStyleOption(&o, idx);

                const QStyle* style = o.widget ? o.widget->style() : QApplication::style();
                o.text.clear(); // we draw text manually
                style->drawControl(QStyle::CE_ItemViewItem, &o, p, o.widget);

                const QRect r = opt.rect;

                // [×] area
                const QRect delR(r.left(), r.top(), kDelW, r.height());
                p->save();
                p->setPen(QColor(200, 60, 60));
                QFont f = p->font();
                f.setBold(true);
                p->setFont(f);
                p->drawText(delR, Qt::AlignCenter, QStringLiteral("×")); // ×
                p->restore();

                // Thin vertical separator after [×]
                p->save();
                p->setPen(QColor(80, 80, 80));
                p->drawLine(r.left() + kDelW, r.top() + 2,
                            r.left() + kDelW, r.bottom() - 2);
                p->restore();

                // Key text
                const QRect textR(r.left() + kDelW + 4, r.top(),
                                  r.width() - kDelW - 6, r.height());
                const QPalette::ColorRole role =
                    (o.state & QStyle::State_Selected) ? QPalette::HighlightedText
                                                        : QPalette::Text;
                style->drawItemText(p, textR,
                    Qt::AlignLeft | Qt::AlignVCenter,
                    o.palette, true, idx.data(Qt::DisplayRole).toString(), role);
            }

            bool editorEvent(QEvent* ev, QAbstractItemModel* /*model*/,
                             const QStyleOptionViewItem& opt,
                             const QModelIndex& idx) override
            {
                if (ev->type() == QEvent::MouseButtonRelease)
                {
                    const QRect delR(opt.rect.left(), opt.rect.top(),
                                     kDelW, opt.rect.height());
                    if (delR.contains(static_cast<QMouseEvent*>(ev)->pos()) && m_deleteFn)
                    {
                        m_deleteFn(idx.data(Qt::DisplayRole).toString());
                        return true;
                    }
                }
                return false;
            }

        private:
            DeleteFn m_deleteFn;
        };
    } // anonymous namespace

    // ── ──────────────────────────────────────────────────────────────────────

    const QString LexiconToolWindow::NewLexiconSentinel = QStringLiteral("__new__");

    static bool IsValidKeyPath(const QString& key)
    {
        if (key.isEmpty()) return false;
        if (key.startsWith('.') || key.endsWith('.')) return false;
        if (key.contains(QStringLiteral(".."))) return false;
        const QStringList segs = key.split('.', Qt::SkipEmptyParts);
        return !segs.isEmpty();
    }

    ////////////////////////////////////////////////////////////////////////
    // Construction

    LexiconToolWindow::LexiconToolWindow(QWidget* parent)
        : QWidget(parent)
    {
        setObjectName("LexiconToolWindow");

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(4, 4, 4, 4);
        root->setSpacing(4);

        // ── Source Files header (always visible) ──────────────────────────────
        m_sourcesPanel = BuildSourceFilesPanel();
        root->addWidget(m_sourcesPanel);

        // ── Splitter: source file list (top) | tab content (bottom) ──────────
        m_sourcesSplitter = new QSplitter(Qt::Vertical, this);
        m_sourcesSplitter->setChildrenCollapsible(false);

        // Source Files scroll area (top pane)
        m_sourcesBody       = new QWidget;
        m_sourcesBodyLayout = new QVBoxLayout(m_sourcesBody);
        m_sourcesBodyLayout->setContentsMargins(4, 2, 4, 2);
        m_sourcesBodyLayout->setSpacing(2);
        m_sourcesBodyLayout->addStretch();

        m_sourcesScrollArea = new QScrollArea;
        m_sourcesScrollArea->setWidget(m_sourcesBody);
        m_sourcesScrollArea->setWidgetResizable(true);
        m_sourcesScrollArea->setMinimumHeight(30);
        m_sourcesScrollArea->setFrameShape(QFrame::StyledPanel);
        m_sourcesSplitter->addWidget(m_sourcesScrollArea);

        // Tab content (bottom pane)
        auto* tabsWidget = new QWidget;
        auto* tabsLay    = new QVBoxLayout(tabsWidget);
        tabsLay->setContentsMargins(0, 0, 0, 0);
        tabsLay->setSpacing(0);

        m_tabBar = new QTabBar(tabsWidget);
        m_tabBar->addTab("Workbench");
        m_tabBar->addTab("Gather");
        m_tabBar->addTab("CSV");
        m_tabBar->setExpanding(false);
        tabsLay->addWidget(m_tabBar);

        m_tabStack = new QStackedWidget(tabsWidget);
        m_tabStack->addWidget(BuildWorkbenchTab());
        m_tabStack->addWidget(BuildGatherTab());
        m_tabStack->addWidget(BuildCsvTab());
        tabsLay->addWidget(m_tabStack, 1);

        m_sourcesSplitter->addWidget(tabsWidget);
        m_sourcesSplitter->setStretchFactor(0, 0);
        m_sourcesSplitter->setStretchFactor(1, 1);
        m_sourcesSplitter->setSizes({ 160, 500 });

        connect(m_sourcesSplitter, &QSplitter::splitterMoved, this, [this](int, int)
        {
            QSettings s;
            s.setValue(QStringLiteral("%1/SourcesSplitterState").arg(SettingsGroup),
                       m_sourcesSplitter->saveState());
        });

        root->addWidget(m_sourcesSplitter, 1);

        connect(m_tabBar, &QTabBar::currentChanged,
                this, &LexiconToolWindow::OnTabChanged);

        // ── File watcher ──────────────────────────────────────────────────────
        m_fileWatcher = new QFileSystemWatcher(this);
        connect(m_fileWatcher, &QFileSystemWatcher::fileChanged,
                this, [this](const QString&)
                {
                    QTimer::singleShot(300, this, [this]() { ReloadModels(); });
                });

        RestoreState();
    }

    ////////////////////////////////////////////////////////////////////////
    // Show / Hide

    void LexiconToolWindow::hideEvent(QHideEvent* event)
    {
        SaveState();
        QWidget::hideEvent(event);
    }

    void LexiconToolWindow::showEvent(QShowEvent* event)
    {
        QWidget::showEvent(event);
        DiscoverFiles();
    }

    ////////////////////////////////////////////////////////////////////////
    // Build helpers

    QWidget* LexiconToolWindow::BuildSourceFilesPanel()
    {
        // Returns just the header row — the scroll area body is created in the constructor
        // and placed in m_sourcesSplitter as the top pane.
        auto* panel  = new QWidget(this);
        auto* layout = new QHBoxLayout(panel);
        layout->setContentsMargins(0, 2, 0, 2);
        layout->setSpacing(4);

        m_sourcesToggle = new QToolButton(panel);
        m_sourcesToggle->setArrowType(m_sourcesExpanded ? Qt::DownArrow : Qt::RightArrow);
        m_sourcesToggle->setFixedSize(18, 18);
        m_sourcesToggle->setToolTip("Toggle Source Files panel");
        connect(m_sourcesToggle, &QToolButton::clicked,
                this, &LexiconToolWindow::OnToggleSourceFiles);

        auto* srcLabel   = new QLabel("<b>Source Files</b>", panel);
        auto* newFileBtn = new QPushButton("New Culture File…", panel);
        newFileBtn->setFixedWidth(130);
        connect(newFileBtn, &QPushButton::clicked,
                this, &LexiconToolWindow::OnNewCultureFile);

        layout->addWidget(m_sourcesToggle);
        layout->addWidget(srcLabel);
        layout->addStretch();
        layout->addWidget(newFileBtn);

        return panel;
    }

    QWidget* LexiconToolWindow::BuildWorkbenchTab()
    {
        auto* tab = new QWidget(this);
        auto* lay = new QVBoxLayout(tab);
        lay->setContentsMargins(2, 0, 2, 2);
        lay->setSpacing(0);

        // ── Models ────────────────────────────────────────────────────────────
        m_treeModel  = new LexiconTreeModel(this);
        m_tableModel = new LexiconTableModel(this);

        // ── Full-width table (takes all space) ────────────────────────────────
        m_tableView = new QTableView(tab);
        m_tableView->setModel(m_tableModel);
        m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_tableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

        // ColKey: custom delegate renders [×] + key text inline
        m_tableView->setItemDelegateForColumn(LexiconTableModel::ColKey,
            new LexiconKeyDelegate(
                [this](const QString& key)
                {
                    const auto answer = QMessageBox::question(
                        this, "Remove Key",
                        QString("Remove \"%1\" from all %2 file(s)?")
                            .arg(key).arg(m_discoveredFiles.size()),
                        QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
                    if (answer != QMessageBox::Yes) return;
                    RemoveKeyFromAllFiles(key);
                    RunSuperSetSync();
                    ReloadModels();
                },
                m_tableView));

        m_tableView->setItemDelegateForColumn(LexiconTableModel::ColType,
            new LexiconHintTypeDelegate(m_tableView));
        m_tableView->setItemDelegateForColumn(LexiconTableModel::ColDefault,
            new LexiconValueDelegate(m_tableView));

        auto* header = m_tableView->horizontalHeader();
        header->setStretchLastSection(false);
        header->setMinimumSectionSize(100);
        header->setSectionResizeMode(LexiconTableModel::ColKey,  QHeaderView::Interactive);
        header->setSectionResizeMode(LexiconTableModel::ColType, QHeaderView::Fixed);
        header->resizeSection(LexiconTableModel::ColKey,  220);
        header->resizeSection(LexiconTableModel::ColType, 100);
        m_tableView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        ApplyValueColumnModes();

        m_tableView->verticalHeader()->hide();
        m_tableView->setAlternatingRowColors(true);
        m_tableView->setShowGrid(false);
        m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
        lay->addWidget(m_tableView, 1);

        // ── Warning label (appears at bottom on key validation error) ─────────
        m_keyWarningLabel = new QLabel(tab);
        m_keyWarningLabel->setStyleSheet("color: #e0a020; font-size: 10px; padding: 2px;");
        m_keyWarningLabel->setWordWrap(true);
        m_keyWarningLabel->hide();
        lay->addWidget(m_keyWarningLabel);

        // ── Key input overlay — sits over the ColKey header section ───────────
        // The overlay replaces the "Key" column header text with a QLineEdit + [+].
        auto* keyOverlay = new QWidget(header);
        auto* koLay      = new QHBoxLayout(keyOverlay);
        koLay->setContentsMargins(2, 1, 2, 1);
        koLay->setSpacing(2);

        m_keyField = new QLineEdit(keyOverlay);
        m_keyField->setPlaceholderText("New key path (e.g. UI.Title)");
        m_keyField->setEnabled(false);
        connect(m_keyField, &QLineEdit::textChanged, this, &LexiconToolWindow::OnKeyFieldChanged);
        connect(m_keyField, &QLineEdit::returnPressed, this, &LexiconToolWindow::OnAddKey);
        koLay->addWidget(m_keyField, 1);

        m_addKeyBtn = new QPushButton("+", keyOverlay);
        m_addKeyBtn->setFixedWidth(26);
        m_addKeyBtn->setEnabled(false);
        connect(m_addKeyBtn, &QPushButton::clicked, this, &LexiconToolWindow::OnAddKey);
        koLay->addWidget(m_addKeyBtn);
        keyOverlay->show();
        keyOverlay->raise();

        // Helper to reposition the overlay over the ColKey header section
        auto reposition = [header, keyOverlay]()
        {
            const int x = header->sectionViewportPosition(LexiconTableModel::ColKey);
            const int w = header->sectionSize(LexiconTableModel::ColKey);
            keyOverlay->setGeometry(x, 0, w, header->height());
        };

        // Reposition on column resize or horizontal scroll
        connect(header, &QHeaderView::sectionResized, this,
                [reposition](int logicalIdx, int, int)
                {
                    if (logicalIdx == LexiconTableModel::ColKey) reposition();
                });
        connect(m_tableView->horizontalScrollBar(), &QScrollBar::valueChanged,
                this, [reposition](int) { reposition(); });

        // Initial position — defer until layout is finalised
        QTimer::singleShot(0, keyOverlay, [reposition]() { reposition(); });

        // ── Connections ───────────────────────────────────────────────────────
        connect(m_tableModel, &LexiconTableModel::TypeChanged,
                this, &LexiconToolWindow::OnTypeChanged);
        connect(m_tableModel, &LexiconTableModel::DefaultValueEdited,
                this, &LexiconToolWindow::OnDefaultValueEdited);
        connect(m_tableModel, &LexiconTableModel::ExtraValueEdited,
                this, &LexiconToolWindow::OnExtraValueEdited);

        // [+] phantom column header → extra column picker
        connect(header, &QHeaderView::sectionClicked,
                this, [this](int section)
                {
                    if (m_tableModel->HasAddColumnButton() &&
                        section == m_tableModel->columnCount() - 1)
                        ShowExtraColumnPicker();
                });

        // Right-click on table row → also offers delete (fallback alongside [×])
        connect(m_tableView, &QTableView::customContextMenuRequested,
                this, &LexiconToolWindow::OnTableContextMenuRequested);

        return tab;
    }

    QWidget* LexiconToolWindow::BuildGatherTab()
    {
        auto* tab = new QWidget(this);
        auto* lay = new QVBoxLayout(tab);
        lay->setContentsMargins(0, 4, 0, 0);

        m_inboxPanel = new LexiconGathererInboxWidget(tab);
        lay->addWidget(m_inboxPanel, 1);

        connect(m_inboxPanel, &LexiconGathererInboxWidget::ItemsAccepted,
                this, &LexiconToolWindow::OnGathererItemsAccepted);

        return tab;
    }

    QWidget* LexiconToolWindow::BuildCsvTab()
    {
        auto* tab = new QWidget(this);
        auto* lay = new QVBoxLayout(tab);
        lay->setContentsMargins(4, 8, 4, 4);
        lay->setSpacing(8);

        // Export section
        auto* exportBox = new QGroupBox("Export", tab);
        auto* exportLay = new QVBoxLayout(exportBox);

        auto* exportOptRow = new QHBoxLayout;
        auto* textOnlyChk = new QCheckBox("Text Only",   exportBox);
        auto* allFilesChk = new QCheckBox("All Files",   exportBox);
        textOnlyChk->setChecked(true);
        allFilesChk->setChecked(true);
        textOnlyChk->setObjectName("csvTextOnly");
        allFilesChk->setObjectName("csvAllFiles");
        exportOptRow->addWidget(textOnlyChk);
        exportOptRow->addWidget(allFilesChk);
        exportOptRow->addStretch();
        exportLay->addLayout(exportOptRow);

        auto* exportBtnRow = new QHBoxLayout;
        auto* exportBtn    = new QPushButton("Export to File…", exportBox);
        auto* copyBtn      = new QPushButton("Copy to Clipboard",   exportBox);
        exportBtnRow->addWidget(exportBtn);
        exportBtnRow->addWidget(copyBtn);
        exportBtnRow->addStretch();
        exportLay->addLayout(exportBtnRow);
        lay->addWidget(exportBox);

        // Import section
        auto* importBox = new QGroupBox("Import", tab);
        auto* importLay = new QHBoxLayout(importBox);
        auto* importBtn = new QPushButton("Load from File…", importBox);
        auto* applyBtn  = new QPushButton("Import",              importBox);
        applyBtn->setObjectName("csvApply");
        applyBtn->setEnabled(false);
        importLay->addWidget(importBtn);
        importLay->addWidget(applyBtn);
        importLay->addStretch();
        lay->addWidget(importBox);

        // Preview
        lay->addWidget(new QLabel("Preview:", tab));
        m_csvPreview = new QTextEdit(tab);
        m_csvPreview->setReadOnly(false);
        m_csvPreview->setPlaceholderText("CSV content will appear here after export or load.");
        m_csvPreview->setFont(QFont(QStringLiteral("Monospace"), 9));
        lay->addWidget(m_csvPreview, 1);

        // Connections
        connect(exportBtn, &QPushButton::clicked, this, &LexiconToolWindow::OnCsvExport);
        connect(copyBtn,   &QPushButton::clicked, this, &LexiconToolWindow::OnCsvCopyToClipboard);
        connect(importBtn, &QPushButton::clicked, this, &LexiconToolWindow::OnCsvImport);
        connect(applyBtn,  &QPushButton::clicked, this, [this]()
        {
            // Write preview content to a temp file then parse it via the standard path
            const QString csvText = m_csvPreview->toPlainText();
            if (csvText.isEmpty()) return;

            const QString tmpPath =
                QDir::temp().filePath(QStringLiteral("helex_import_tmp.csv"));
            QFile tmp(tmpPath);
            if (!tmp.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                QMessageBox::warning(this, "Import CSV",
                    "Could not write temporary file for import.");
                return;
            }
            tmp.write(csvText.toUtf8());
            tmp.close();

            const LexiconCsvInterop::MultiImportResult result =
                LexiconCsvInterop::ParseMulti(tmpPath);
            QFile::remove(tmpPath);

            if (!result.errors.isEmpty())
            {
                QMessageBox::warning(this, "Import CSV",
                    result.errors.join(QLatin1Char('\n')));
                return;
            }

            for (const auto& col : result.columns)
            {
                if (col.filePath.isEmpty() || col.pairs.isEmpty()) continue;
                QString targetPath;
                for (const QString& dp : m_discoveredFiles)
                    if (QFileInfo(dp).fileName().compare(col.filePath, Qt::CaseInsensitive) == 0)
                    { targetPath = dp; break; }
                if (targetPath.isEmpty()) continue;
                for (auto it = col.pairs.cbegin(); it != col.pairs.cend(); ++it)
                    WriteValueToFile(targetPath, it.key(), QJsonValue(it.value()));
            }

            RunSuperSetSync();
            ReloadModels();
        });
        connect(m_csvPreview, &QTextEdit::textChanged, this, [this, applyBtn]()
        {
            applyBtn->setEnabled(!m_csvPreview->toPlainText().isEmpty());
        });

        return tab;
    }

    ////////////////////////////////////////////////////////////////////////
    // Source Files panel management

    void LexiconToolWindow::OnToggleSourceFiles()
    {
        m_sourcesExpanded = !m_sourcesExpanded;
        m_sourcesToggle->setArrowType(m_sourcesExpanded ? Qt::DownArrow : Qt::RightArrow);
        if (m_sourcesScrollArea)
            m_sourcesScrollArea->setVisible(m_sourcesExpanded);
    }

    void LexiconToolWindow::RebuildSourceFileEntries()
    {
        while (QLayoutItem* item = m_sourcesBodyLayout->takeAt(0))
        {
            if (item->widget()) item->widget()->deleteLater();
            delete item;
        }

        for (int i = 0; i < m_fileEntries.size(); ++i)
        {
            const HelexFileEntry& fe = m_fileEntries[i];

            const bool isDefault =
                fe.assetId.compare("default", Qt::CaseInsensitive) == 0 ||
                QFileInfo(fe.path).baseName().compare("default", Qt::CaseInsensitive) == 0;

            // ── Single horizontal row per file ────────────────────────────────
            // | Name (bold, 90px) | [chips...] (inactive hint) | [input] [+] |
            auto* row    = new QWidget(m_sourcesBody);
            auto* rowLay = new QHBoxLayout(row);
            rowLay->setContentsMargins(4, 2, 4, 2);
            rowLay->setSpacing(6);

            // File name (fixed width so all rows align)
            const QString displayName = fe.assetId.isEmpty()
                ? QFileInfo(fe.path).baseName() : fe.assetId;
            auto* nameLabel = new QLabel(
                QString("<b>%1:</b>").arg(displayName), row);
            nameLabel->setFixedWidth(90);
            nameLabel->setToolTip(fe.path);
            rowLay->addWidget(nameLabel);

            const int capturedIdx = i;

            // Culture chips inline
            for (const QString& code : fe.cultures)
            {
                auto* chip = new QPushButton(code + " ×", row);
                chip->setFlat(true);
                chip->setStyleSheet(
                    "QPushButton { background:#2a5f8a; color:#fff; border-radius:3px;"
                    " padding:1px 5px; font-size:11px; max-height:20px; }"
                    "QPushButton:hover { background:#cc3333; }");
                const QString capturedCode = code;
                connect(chip, &QPushButton::clicked, this, [this, capturedIdx, capturedCode]()
                {
                    RemoveCultureFromFile(capturedIdx, capturedCode);
                });
                rowLay->addWidget(chip);
            }

            // Inactive hint — non-default files with no cultures need at least one
            if (fe.cultures.isEmpty() && !isDefault)
            {
                auto* hint = new QLabel(
                    "<i style='color:#888; font-size:10px;'>(add a culture)</i>", row);
                rowLay->addWidget(hint);
            }

            // Input + [+] button sit immediately after the last chip, then stretch fills the rest
            auto* cultureInput = new QLineEdit(row);
            cultureInput->setPlaceholderText("e.g. fr, French…");
            cultureInput->setMinimumWidth(110);
            cultureInput->setMaximumWidth(180);
            cultureInput->setFixedHeight(22);
            rowLay->addWidget(cultureInput);

            auto* addBtn = new QPushButton("+", row);
            addBtn->setFixedSize(24, 22);
            addBtn->setToolTip("Search for and add a culture code");
            rowLay->addWidget(addBtn);

            rowLay->addStretch(1);

            connect(addBtn, &QPushButton::clicked, this, [this, capturedIdx, cultureInput]()
            {
                ShowCulturePicker(capturedIdx, cultureInput->text().trimmed());
                cultureInput->clear();
            });
            connect(cultureInput, &QLineEdit::returnPressed, addBtn, &QPushButton::click);

            m_sourcesBodyLayout->addWidget(row);
        }

        m_sourcesBodyLayout->addStretch();
    }

    void LexiconToolWindow::AddCultureToFile(int fileIdx, const QString& code)
    {
        if (fileIdx < 0 || fileIdx >= m_fileEntries.size() || code.isEmpty())
            return;
        if (m_fileEntries[fileIdx].cultures.contains(code))
            return;

        m_fileEntries[fileIdx].cultures.append(code);

        // Write back to the .helex file
        const QString& path = m_fileEntries[fileIdx].path;
        QFile f(path);
        if (f.open(QIODevice::ReadOnly))
        {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
            f.close();
            if (err.error == QJsonParseError::NoError && doc.isObject())
            {
                QJsonObject root = doc.object();
                QJsonArray cultures;
                for (const QString& c : m_fileEntries[fileIdx].cultures)
                    cultures.append(c);
                root[QLatin1String("cultures")] = cultures;
                doc.setObject(root);
                if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
                {
                    f.write(doc.toJson(QJsonDocument::Indented));
                    f.close();
                }
            }
        }

        RebuildSourceFileEntries();
    }

    void LexiconToolWindow::RemoveCultureFromFile(int fileIdx, const QString& code)
    {
        if (fileIdx < 0 || fileIdx >= m_fileEntries.size())
            return;

        m_fileEntries[fileIdx].cultures.removeAll(code);

        const QString& path = m_fileEntries[fileIdx].path;
        QFile f(path);
        if (f.open(QIODevice::ReadOnly))
        {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
            f.close();
            if (err.error == QJsonParseError::NoError && doc.isObject())
            {
                QJsonObject root = doc.object();
                QJsonArray cultures;
                for (const QString& c : m_fileEntries[fileIdx].cultures)
                    cultures.append(c);
                root[QLatin1String("cultures")] = cultures;
                doc.setObject(root);
                if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
                {
                    f.write(doc.toJson(QJsonDocument::Indented));
                    f.close();
                }
            }
        }

        RebuildSourceFileEntries();
    }

    void LexiconToolWindow::ShowCulturePicker(int fileIdx, const QString& filter)
    {
        auto* menu = new QMenu(this);
        const QString f = filter.toLower();
        int shown = 0;

        for (const auto& kc : KnownCulturesTable)
        {
            if (!f.isEmpty())
            {
                const QString code = QString::fromUtf8(kc.code.data(), (int)kc.code.size());
                const QString name = QString::fromUtf8(kc.name.data(), (int)kc.name.size());
                if (!code.startsWith(f, Qt::CaseInsensitive) &&
                    !name.contains(f, Qt::CaseInsensitive))
                    continue;
            }

            const QString code = QString::fromUtf8(kc.code.data(), (int)kc.code.size());
            const QString name = QString::fromUtf8(kc.name.data(), (int)kc.name.size());

            auto* action = menu->addAction(
                QString("%1  —  %2").arg(code, name));
            const QString capturedCode = code;
            connect(action, &QAction::triggered, this, [this, fileIdx, capturedCode]()
            {
                AddCultureToFile(fileIdx, capturedCode);
            });

            if (++shown >= 50) break;
        }

        if (shown == 0)
        {
            if (!filter.isEmpty())
            {
                auto* action = menu->addAction(
                    QString("Add \"%1\" (custom code)").arg(filter));
                const QString capturedFilter = filter;
                connect(action, &QAction::triggered, this, [this, fileIdx, capturedFilter]()
                {
                    AddCultureToFile(fileIdx, capturedFilter);
                });
            }
            else
            {
                menu->addAction("(all known cultures already added)")->setEnabled(false);
            }
        }

        menu->exec(QCursor::pos());
    }

    void LexiconToolWindow::OnNewCultureFile()
    {
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

        QJsonArray culturesArray;
        for (const QString& code : cultures.split(QLatin1Char(','), Qt::SkipEmptyParts))
            culturesArray.append(code.trimmed());

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
            QMessageBox::warning(this, "New Culture File Failed",
                QString("Could not write file:\n%1").arg(path));
            return;
        }
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();

        LexiconEditorRequestBus::Broadcast(&LexiconEditorRequests::RefreshKeyTree);
        DiscoverFiles();

        RunSuperSetSync();
        ReloadModels();
    }

    ////////////////////////////////////////////////////////////////////////
    // Tab switching

    void LexiconToolWindow::OnTabChanged(int index)
    {
        m_tabStack->setCurrentIndex(index);

        if (index == 1) // Gather
        {
            const QString defaultPath = [this]() -> QString {
                for (const QString& p : m_discoveredFiles)
                    if (QFileInfo(p).baseName().compare("default", Qt::CaseInsensitive) == 0 ||
                        m_fileAssetIds.value(p).compare("default", Qt::CaseInsensitive) == 0)
                        return p;
                return m_discoveredFiles.isEmpty() ? QString{} : m_discoveredFiles.first();
            }();
            if (m_inboxPanel && !defaultPath.isEmpty())
                m_inboxPanel->SetTargetHelexPath(defaultPath);
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // Add Key bar

    void LexiconToolWindow::OnKeyFieldChanged(const QString& text)
    {
        m_keyWarningLabel->hide();
        const bool valid = IsValidKeyPath(text.trimmed());
        m_addKeyBtn->setEnabled(valid && !m_discoveredFiles.isEmpty());
    }

    void LexiconToolWindow::OnAddKey()
    {
        m_keyWarningLabel->hide();

        if (m_discoveredFiles.isEmpty())
        {
            m_keyWarningLabel->setText(
                "⚠ No .helex files found. Use ‘New Culture File…’ to create one.");
            m_keyWarningLabel->show();
            return;
        }

        const QString key = m_keyField->text().trimmed();

        if (!IsValidKeyPath(key))
        {
            m_keyWarningLabel->setText(
                "Invalid key — check for leading, trailing, or double dots.");
            m_keyWarningLabel->show();
            return;
        }

        if (m_treeModel->EntryMap().contains(key))
        {
            m_keyWarningLabel->setText(
                QString("⚠ Key \"%1\" already exists.").arg(key));
            m_keyWarningLabel->show();
            return;
        }

        WriteKeyToAllFiles(key);
        RunSuperSetSync();
        ReloadModels();

        if (!m_treeModel->EntryMap().contains(key))
        {
            m_keyWarningLabel->setText(
                QString("⚠ Key \"%1\" was not found after reload.").arg(key));
            m_keyWarningLabel->show();
        }

        m_keyField->clear();
    }

    ////////////////////////////////////////////////////////////////////////
    // Table signal handlers

    void LexiconToolWindow::OnTypeChanged(const QString& key, LexiconHintType newHint)
    {
        // Update the type hint in all discovered .helex files for this key
        for (const QString& path : m_discoveredFiles)
            WriteTypeToFile(path, key, newHint);

        ReloadModels();
    }

    void LexiconToolWindow::OnDefaultValueEdited(const QString& key, const QString& newValue)
    {
        const QString defaultPath = [this]() -> QString {
            for (const QString& p : m_discoveredFiles)
                if (QFileInfo(p).baseName().compare("default", Qt::CaseInsensitive) == 0 ||
                    m_fileAssetIds.value(p).compare("default", Qt::CaseInsensitive) == 0)
                    return p;
            return m_discoveredFiles.isEmpty() ? QString{} : m_discoveredFiles.first();
        }();

        if (defaultPath.isEmpty()) return;

        if (!WriteValueToFile(defaultPath, key, QJsonValue(newValue)))
            QMessageBox::warning(this, "Write Failed",
                QString("Could not write to default file:\n%1").arg(defaultPath));
        else
            ReloadModels();
    }

    void LexiconToolWindow::OnExtraValueEdited(const QString& key, int extraColIndex,
                                                const QString& newValue)
    {
        if (extraColIndex < 0 || extraColIndex >= m_extraColumnPaths.size())
            return;

        const QString& path = m_extraColumnPaths[extraColIndex];
        if (!WriteValueToFile(path, key, QJsonValue(newValue)))
            QMessageBox::warning(this, "Write Failed",
                QString("Could not write to:\n%1").arg(path));
        else
            ReloadModels();
    }

    ////////////////////////////////////////////////////////////////////////
    // Extra column picker

    void LexiconToolWindow::ShowExtraColumnPicker()
    {
        auto* menu = new QMenu(this);

        for (const QString& path : m_discoveredFiles)
        {
            const QString assetId = m_fileAssetIds.value(path, QFileInfo(path).baseName());
            const bool alreadyShown = m_extraColumnPaths.contains(path);

            auto* action = menu->addAction(assetId);
            action->setCheckable(true);
            action->setChecked(alreadyShown);
            const QString capturedPath = path;
            connect(action, &QAction::toggled, this, [this, capturedPath](bool on)
            {
                if (on) AddExtraColumn(capturedPath);
                else    RemoveExtraColumn(capturedPath);
            });
        }

        if (m_discoveredFiles.isEmpty())
            menu->addAction("(no other .helex files found)")->setEnabled(false);

        menu->exec(QCursor::pos());
    }

    void LexiconToolWindow::AddExtraColumn(const QString& filePath)
    {
        if (m_extraColumnPaths.contains(filePath)) return;
        m_extraColumnPaths.append(filePath);
        ReloadModels();
    }

    void LexiconToolWindow::RemoveExtraColumn(const QString& filePath)
    {
        m_extraColumnPaths.removeAll(filePath);
        ReloadModels();
    }

    ////////////////////////////////////////////////////////////////////////
    // Table context menu — right-click to delete a key

    void LexiconToolWindow::OnTableContextMenuRequested(const QPoint& pos)
    {
        const QModelIndex idx = m_tableView->indexAt(pos);
        if (!idx.isValid()) return;

        const QString key = m_tableModel->data(
            m_tableModel->index(idx.row(), LexiconTableModel::ColKey),
            Qt::DisplayRole).toString();
        if (key.isEmpty()) return;

        QMenu menu(this);
        auto* del = menu.addAction(QString("Remove Key \"%1\"…").arg(key));
        connect(del, &QAction::triggered, this, [this, key]()
        {
            const auto answer = QMessageBox::question(
                this, "Remove Key",
                QString("Remove \"%1\" from all %2 file(s)?\n\nThis cannot be undone.")
                    .arg(key).arg(m_discoveredFiles.size()),
                QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
            if (answer != QMessageBox::Yes) return;
            RemoveKeyFromAllFiles(key);
            RunSuperSetSync();
            ReloadModels();
        });
        menu.exec(m_tableView->viewport()->mapToGlobal(pos));
    }

    ////////////////////////////////////////////////////////////////////////
    // File changed (watcher)

    void LexiconToolWindow::OnFileChanged(const QString& /*path*/)
    {
        QTimer::singleShot(300, this, [this]() { ReloadModels(); });
    }

    ////////////////////////////////////////////////////////////////////////
    // Gatherer

    void LexiconToolWindow::OnGathererItemsAccepted([[maybe_unused]] const QStringList& keys)
    {
        RunSuperSetSync();
        ReloadModels();
    }

    ////////////////////////////////////////////////////////////////////////
    // CSV tab slots

    void LexiconToolWindow::OnCsvExport()
    {
        if (m_discoveredFiles.isEmpty())
        {
            QMessageBox::information(this, "Export CSV", "No .helex files found.");
            return;
        }

        QString savePath = QFileDialog::getSaveFileName(
            this, "Export CSV", m_localisationDir, "CSV Files (*.csv)");
        if (savePath.isEmpty()) return;
        if (!savePath.endsWith(QLatin1String(".csv"), Qt::CaseInsensitive))
            savePath += QLatin1String(".csv");

        // Build multi-file export
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
        {
            // Load the exported file into the preview textarea
            QFile pf(savePath);
            if (pf.open(QIODevice::ReadOnly))
            {
                m_csvPreview->setPlainText(QString::fromUtf8(pf.readAll()));
                pf.close();
            }
            QMessageBox::information(this, "Export CSV",
                QString("Exported to:\n%1").arg(savePath));
        }
    }

    void LexiconToolWindow::OnCsvCopyToClipboard()
    {
        const QString text = m_csvPreview->toPlainText();
        if (!text.isEmpty())
            QApplication::clipboard()->setText(text);
    }

    void LexiconToolWindow::OnCsvImport()
    {
        const QString csvPath = QFileDialog::getOpenFileName(
            this, "Import CSV", m_localisationDir,
            "CSV Files (*.csv);;All Files (*)");
        if (csvPath.isEmpty()) return;

        QFile f(csvPath);
        if (f.open(QIODevice::ReadOnly))
        {
            m_csvPreview->setPlainText(QString::fromUtf8(f.readAll()));
            f.close();
        }
    }


    ////////////////////////////////////////////////////////////////////////
    // File operations (mostly unchanged from original)

    void LexiconToolWindow::DiscoverFiles()
    {
        const QStringList previousFiles = m_discoveredFiles;

        m_discoveredFiles.clear();
        m_fileAssetIds.clear();
        m_fileEntries.clear();

        AZStd::string sourcePath;
        LexiconEditorRequestBus::BroadcastResult(
            sourcePath, &LexiconEditorRequests::GetProjectSourcePath);
        const QString projectDir = QString::fromUtf8(sourcePath.c_str());

        auto scanForFiles = [this, &projectDir]()
        {
            if (projectDir.isEmpty()) return;
            QDirIterator it(projectDir,
                            QStringList() << QStringLiteral("*.helex"),
                            QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext())
                m_discoveredFiles.append(it.next());
        };

        scanForFiles();

        if (m_discoveredFiles.isEmpty())
        {
            CreateDefaultLexicon();
            scanForFiles();
        }

        m_discoveredFiles.sort(Qt::CaseInsensitive);

        m_localisationDir.clear();
        for (const QString& path : m_discoveredFiles)
        {
            HelexFileEntry fe;
            fe.path = path;
            fe.assetId = QFileInfo(path).baseName();

            QFile file(path);
            if (file.open(QIODevice::ReadOnly))
            {
                QJsonParseError err;
                const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
                file.close();
                if (err.error == QJsonParseError::NoError && doc.isObject())
                {
                    const QJsonObject root = doc.object();
                    const QString id = root.value(QLatin1String("assetId")).toString().trimmed();
                    if (!id.isEmpty()) fe.assetId = id;

                    const QJsonArray cultureArr = root.value(QLatin1String("cultures")).toArray();
                    for (const QJsonValue& v : cultureArr)
                        fe.cultures.append(v.toString().trimmed());
                }
            }
            m_fileAssetIds[path] = fe.assetId;
            m_fileEntries.append(fe);

            if (m_localisationDir.isEmpty() &&
                QFileInfo(path).baseName().compare("default", Qt::CaseInsensitive) == 0)
                m_localisationDir = QFileInfo(path).absolutePath();
        }

        if (m_localisationDir.isEmpty() && !m_discoveredFiles.isEmpty())
            m_localisationDir = QFileInfo(m_discoveredFiles.first()).absolutePath();

        if (!previousFiles.isEmpty())
        {
            QStringList newFiles;
            for (const QString& path : m_discoveredFiles)
                if (!previousFiles.contains(path))
                    newFiles.append(path);
            if (!newFiles.isEmpty())
                RunCrossPollination(newFiles);
        }

        // Update key field enabled state
        const bool hasFiles = !m_discoveredFiles.isEmpty();
        if (m_keyField)   m_keyField->setEnabled(hasFiles);

        RebuildSourceFileEntries();
        RunSuperSetSync();
        ReloadModels();
    }

    void LexiconToolWindow::RunCrossPollination(const QStringList& newFiles)
    {
        QMap<QString, QSet<QString>> toInject;

        for (const QString& newPath : newFiles)
        {
            const QString newAssetId =
                m_fileAssetIds.value(newPath, QFileInfo(newPath).baseName());

            for (const QString& otherPath : m_discoveredFiles)
            {
                if (otherPath == newPath) continue;
                const QString otherAssetId =
                    m_fileAssetIds.value(otherPath, QFileInfo(otherPath).baseName());

                toInject[newPath].insert(QStringLiteral("Language.") + otherAssetId);
                toInject[otherPath].insert(QStringLiteral("Language.") + newAssetId);
            }
        }

        if (toInject.isEmpty()) return;

        QMap<QString, QStringList> pending;
        for (auto it = toInject.cbegin(); it != toInject.cend(); ++it)
        {
            QStringList missing;
            QFile f(it.key());
            if (!f.open(QIODevice::ReadOnly)) continue;
            QJsonParseError err;
            const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
            f.close();
            if (err.error != QJsonParseError::NoError || !doc.isObject()) continue;

            const QJsonObject entries =
                doc.object().value(QLatin1String("entries")).toObject();
            for (const QString& key : it.value())
                if (!entries.contains(key))
                    missing << key;
            missing.sort();
            if (!missing.isEmpty())
                pending.insert(it.key(), missing);
        }

        if (pending.isEmpty()) return;

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
        dlgLayout->addWidget(new QLabel(
            "New .helex file(s) detected.\nThe following Language.* keys will be injected:", &dlg));
        auto* diffView = new QTextEdit(&dlg);
        diffView->setReadOnly(true);
        diffView->setPlainText(diffText);
        diffView->setMinimumSize(480, 180);
        dlgLayout->addWidget(diffView);
        auto* dlgButtons = new QDialogButtonBox(&dlg);
        auto* applyBtn   = dlgButtons->addButton("Apply", QDialogButtonBox::AcceptRole);
        dlgButtons->addButton("Skip", QDialogButtonBox::RejectRole);
        Q_UNUSED(applyBtn);
        connect(dlgButtons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(dlgButtons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        dlgLayout->addWidget(dlgButtons);

        if (dlg.exec() != QDialog::Accepted) return;

        for (auto it = pending.cbegin(); it != pending.cend(); ++it)
            for (const QString& key : it.value())
                WriteValueToFile(it.key(), key, QJsonValue(QString{}));
    }

    int LexiconToolWindow::RunSuperSetSync()
    {
        if (m_discoveredFiles.isEmpty()) return 0;

        QSet<QString> allKeys;
        for (const QString& path : m_discoveredFiles)
        {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) continue;
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
            file.close();
            if (err.error != QJsonParseError::NoError || !doc.isObject()) continue;
            const QJsonObject entries =
                doc.object().value(QLatin1String("entries")).toObject();
            for (auto it = entries.begin(); it != entries.end(); ++it)
                allKeys.insert(it.key());
        }

        // Ensure Language.<assetId> for every file
        for (const QString& path : m_discoveredFiles)
        {
            QFile idFile(path);
            if (!idFile.open(QIODevice::ReadOnly)) continue;
            QJsonParseError idErr;
            QJsonDocument idDoc = QJsonDocument::fromJson(idFile.readAll(), &idErr);
            idFile.close();
            if (idErr.error != QJsonParseError::NoError || !idDoc.isObject()) continue;
            const QString assetId =
                idDoc.object().value(QLatin1String("assetId")).toString().trimmed();
            if (!assetId.isEmpty())
                allKeys.insert(QStringLiteral("Language.") + assetId);
        }

        if (allKeys.isEmpty()) return 0;

        int patchedFiles = 0;
        for (const QString& path : m_discoveredFiles)
        {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) continue;
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
            file.close();
            if (err.error != QJsonParseError::NoError || !doc.isObject()) continue;

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
            if (!file.open(QIODevice::ReadOnly)) continue;
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
            file.close();
            if (err.error != QJsonParseError::NoError || !doc.isObject()) continue;

            QJsonObject root    = doc.object();
            QJsonObject entries = root.value(QLatin1String("entries")).toObject();
            if (entries.contains(key)) continue;

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
            if (!file.open(QIODevice::ReadOnly)) continue;
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
            file.close();
            if (err.error != QJsonParseError::NoError || !doc.isObject()) continue;

            QJsonObject root    = doc.object();
            QJsonObject entries = root.value(QLatin1String("entries")).toObject();
            if (!entries.contains(key)) continue;

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
        if (keys.isEmpty()) return;

        for (const QString& path : m_discoveredFiles)
        {
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) continue;
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
            file.close();
            if (err.error != QJsonParseError::NoError || !doc.isObject()) continue;

            QJsonObject root    = doc.object();
            QJsonObject entries = root.value(QLatin1String("entries")).toObject();
            bool modified = false;
            for (const QString& key : keys)
                if (entries.contains(key)) { entries.remove(key); modified = true; }

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

    void LexiconToolWindow::CreateDefaultLexicon()
    {
        AZStd::string sourcePath;
        LexiconEditorRequestBus::BroadcastResult(
            sourcePath, &LexiconEditorRequests::GetProjectSourcePath);
        if (sourcePath.empty()) return;

        QString dir = QString::fromUtf8(sourcePath.c_str());
        if (!dir.endsWith(QLatin1Char('/'))) dir += QLatin1Char('/');
        dir += QLatin1String("Assets/Localisation");

        if (!QDir{dir}.exists() && !QDir{}.mkpath(dir)) return;

        const QString defaultPath = dir + QLatin1String("/default.helex");
        QJsonObject root;
        root[QLatin1String("assetId")]  = QStringLiteral("default");
        root[QLatin1String("cultures")] = QJsonArray{};
        root[QLatin1String("entries")]  = QJsonObject{};

        QFile file(defaultPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();

        LexiconEditorRequestBus::Broadcast(&LexiconEditorRequests::RefreshKeyTree);
    }

    bool LexiconToolWindow::WriteValueToFile(const QString& filePath,
                                               const QString& key,
                                               const QJsonValue& value)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) return false;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
        file.close();
        if (err.error != QJsonParseError::NoError || !doc.isObject()) return false;

        QJsonObject root    = doc.object();
        QJsonObject entries = root.value(QLatin1String("entries")).toObject();
        entries[key] = value;
        root[QLatin1String("entries")] = entries;
        doc.setObject(root);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        return true;
    }

    void LexiconToolWindow::WriteTypeToFile(const QString& filePath,
                                              const QString& key,
                                              LexiconHintType hint)
    {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly)) return;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
        file.close();
        if (err.error != QJsonParseError::NoError || !doc.isObject()) return;

        QJsonObject root    = doc.object();
        QJsonObject entries = root.value(QLatin1String("entries")).toObject();

        if (!entries.contains(key)) return;

        const QJsonValue existing = entries[key];
        const QString hintStr =
            LexiconTableModel::HintTypeLabel(hint).toLower();

        if (hint == LexiconHintType::String)
        {
            // Plain string — write as bare string value
            const QString currentVal = existing.isObject()
                ? existing.toObject().value(QLatin1String("value")).toString()
                : existing.toString();
            entries[key] = currentVal;
        }
        else
        {
            // Asset-type entry — preserve existing uuid/path if present
            QJsonObject entryObj = existing.isObject() ? existing.toObject() : QJsonObject{};
            entryObj[QLatin1String("hint")] = hintStr;
            if (!entryObj.contains(QLatin1String("uuid")))
                entryObj[QLatin1String("uuid")] = QString{};
            entries[key] = entryObj;
        }

        root[QLatin1String("entries")] = entries;
        doc.setObject(root);

        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            file.write(doc.toJson(QJsonDocument::Indented));
            file.close();
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // Column layout

    void LexiconToolWindow::ApplyValueColumnModes()
    {
        if (!m_tableView || !m_tableModel) return;
        auto* hdr = m_tableView->horizontalHeader();
        const int totalCols = m_tableModel->columnCount();
        const int extraEnd  = totalCols - (m_tableModel->HasAddColumnButton() ? 1 : 0);

        for (int col = LexiconTableModel::ColDefault; col < extraEnd; ++col)
            hdr->setSectionResizeMode(col, QHeaderView::Stretch);

        if (m_tableModel->HasAddColumnButton())
        {
            const int plusCol = totalCols - 1;
            hdr->setSectionResizeMode(plusCol, QHeaderView::Fixed);
            hdr->resizeSection(plusCol, 28);
        }
    }

    ////////////////////////////////////////////////////////////////////////
    // Reload models

    void LexiconToolWindow::ReloadModels()
    {
        // Find the default file
        QString defaultPath;
        for (const QString& p : m_discoveredFiles)
        {
            if (QFileInfo(p).baseName().compare("default", Qt::CaseInsensitive) == 0 ||
                m_fileAssetIds.value(p).compare("default", Qt::CaseInsensitive) == 0)
            {
                defaultPath = p;
                break;
            }
        }
        if (defaultPath.isEmpty() && !m_discoveredFiles.isEmpty())
            defaultPath = m_discoveredFiles.first();

        // Load the default file into the tree model
        m_treeModel->LoadFiles(QString{}, defaultPath);

        // Populate the table model with default + hint types
        LexiconEntryMap validatedEntries = m_treeModel->EntryMap();

        // Populate hintType for each entry from the editor bus
        for (auto it = validatedEntries.begin(); it != validatedEntries.end(); ++it)
        {
            LexiconHintType hint = LexiconHintType::None;
            LexiconEditorRequestBus::BroadcastResult(
                hint, &LexiconEditorRequests::GetHintForKey,
                AZStd::string(it.key().toUtf8().constData()));
            it.value().hintType = hint;
        }

        m_tableModel->SetDefaultEntries(validatedEntries);

        // Build extra columns
        QVector<LexiconTableModel::ExtraColumn> extraCols;
        for (const QString& extraPath : m_extraColumnPaths)
        {
            if (!m_discoveredFiles.contains(extraPath)) continue;

            LexiconTableModel::ExtraColumn col;
            col.filePath    = extraPath;
            col.displayName = m_fileAssetIds.value(extraPath, QFileInfo(extraPath).baseName());

            QFile f(extraPath);
            if (f.open(QIODevice::ReadOnly))
            {
                QJsonParseError perr;
                const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &perr);
                f.close();
                if (perr.error == QJsonParseError::NoError && doc.isObject())
                {
                    const QJsonObject entries =
                        doc.object().value(QLatin1String("entries")).toObject();
                    for (auto it = entries.begin(); it != entries.end(); ++it)
                    {
                        const QString val = it.value().isObject()
                            ? it.value().toObject().value(QLatin1String("value")).toString()
                            : it.value().toString();
                        col.values.insert(it.key(), val);
                    }
                }
            }
            extraCols.append(col);
        }
        m_tableModel->SetExtraColumns(extraCols);

        // Show [+] column header only when there are additional files the user could add
        const int potentialExtraCols = m_discoveredFiles.size() - 1; // exclude default
        m_tableModel->SetShowAddColumn(potentialExtraCols > (int)m_extraColumnPaths.size());
        ApplyValueColumnModes();

        // Update file watcher paths
        if (m_fileWatcher)
        {
            if (!m_fileWatcher->files().isEmpty())
                m_fileWatcher->removePaths(m_fileWatcher->files());
            if (!m_discoveredFiles.isEmpty())
                m_fileWatcher->addPaths(m_discoveredFiles);
        }

        // Update key field enabled state
        const bool hasFiles = !m_discoveredFiles.isEmpty();
        if (m_keyField) m_keyField->setEnabled(hasFiles);
        if (m_addKeyBtn)
        {
            const QString key = m_keyField ? m_keyField->text().trimmed() : QString{};
            m_addKeyBtn->setEnabled(hasFiles && IsValidKeyPath(key));
        }

        // Update gatherer target
        if (m_inboxPanel && !defaultPath.isEmpty())
            m_inboxPanel->SetTargetHelexPath(defaultPath);
    }

    ////////////////////////////////////////////////////////////////////////
    // Settings persistence

    void LexiconToolWindow::SaveState()
    {
        QSettings settings;
        settings.beginGroup(SettingsGroup);
        if (m_sourcesSplitter)
            settings.setValue("SourcesSplitterState", m_sourcesSplitter->saveState());
        settings.setValue("TabIndex",        m_tabBar ? m_tabBar->currentIndex() : 0);
        settings.setValue("SourcesExpanded", m_sourcesExpanded);
        settings.setValue("ExtraColumns",    m_extraColumnPaths);
        settings.endGroup();
    }

    void LexiconToolWindow::RestoreState()
    {
        QSettings settings;
        settings.beginGroup(SettingsGroup);

        const QByteArray sourcesSplitterState = settings.value("SourcesSplitterState").toByteArray();
        if (!sourcesSplitterState.isEmpty() && m_sourcesSplitter)
            m_sourcesSplitter->restoreState(sourcesSplitterState);

        const int tabIdx = settings.value("TabIndex", 0).toInt();
        if (m_tabBar && tabIdx >= 0 && tabIdx < m_tabBar->count())
        {
            m_tabBar->setCurrentIndex(tabIdx);
            m_tabStack->setCurrentIndex(tabIdx);
        }

        m_sourcesExpanded = settings.value("SourcesExpanded", true).toBool();
        if (m_sourcesScrollArea)
            m_sourcesScrollArea->setVisible(m_sourcesExpanded);
        if (m_sourcesToggle)
            m_sourcesToggle->setArrowType(m_sourcesExpanded ? Qt::DownArrow : Qt::RightArrow);

        const QStringList extras = settings.value("ExtraColumns").toStringList();
        m_extraColumnPaths = extras;

        settings.endGroup();
    }

    ////////////////////////////////////////////////////////////////////////

} // namespace FoundationLocalisation

#include <moc_LexiconToolWindow.cpp>
