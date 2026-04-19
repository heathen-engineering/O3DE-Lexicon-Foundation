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

#include "LexiconInspectorWidget.h"

#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace FoundationLocalisation
{
    // Image extensions we'll attempt to load as QPixmap thumbnails
    static const QSet<QString> s_imageExtensions = {
        "png", "jpg", "jpeg", "bmp", "gif", "tga", "tif", "tiff", "webp"
    };

    // Audio extensions we recognise (type labelling only)
    static const QSet<QString> s_audioExtensions = {
        "wav", "ogg", "mp3", "wem", "flac", "aif", "aiff"
    };

    static constexpr int  ThumbSize     = 128;
    static constexpr int  DebounceMs    = 500;

    ////////////////////////////////////////////////////////////////////////
    // Construction

    LexiconInspectorWidget::LexiconInspectorWidget(QWidget* parent)
        : QWidget(parent)
    {
        // Debounce timer — single-shot, fires 500 ms after the last keystroke.
        m_debounceTimer = new QTimer(this);
        m_debounceTimer->setSingleShot(true);
        m_debounceTimer->setInterval(DebounceMs);
        connect(m_debounceTimer, &QTimer::timeout,
                this, &LexiconInspectorWidget::OnDebounceTimeout);

        auto* root = new QVBoxLayout(this);
        root->setContentsMargins(0, 0, 0, 0);
        root->setSpacing(0);

        m_stack = new QStackedWidget(this);
        root->addWidget(m_stack);

        // ---- Page 0: Empty -----------------------------------------------
        {
            m_emptyPage = new QWidget();
            auto* l = new QVBoxLayout(m_emptyPage);
            l->setContentsMargins(12, 12, 12, 12);

            auto* lbl = new QLabel("Select a key to inspect.", m_emptyPage);
            lbl->setStyleSheet("color: #666;");
            lbl->setAlignment(Qt::AlignCenter);
            l->addStretch(1);
            l->addWidget(lbl, 0, Qt::AlignCenter);
            l->addStretch(1);

            m_stack->addWidget(m_emptyPage); // index 0
        }

        // ---- Page 1: Namespace -------------------------------------------
        {
            m_namespacePage = new QWidget();
            auto* l = new QVBoxLayout(m_namespacePage);
            l->setContentsMargins(12, 12, 12, 12);
            l->setSpacing(6);

            m_nsPathLabel = new QLabel(m_namespacePage);
            m_nsPathLabel->setWordWrap(true);
            QFont bold = m_nsPathLabel->font();
            bold.setBold(true);
            m_nsPathLabel->setFont(bold);
            l->addWidget(m_nsPathLabel);

            m_nsCountLabel = new QLabel(m_namespacePage);
            m_nsCountLabel->setStyleSheet("color: #888;");
            l->addWidget(m_nsCountLabel);

            l->addStretch(1);
            m_stack->addWidget(m_namespacePage); // index 1
        }

        // ---- Page 2: String entry ----------------------------------------
        {
            auto* scrollArea = new QScrollArea();
            scrollArea->setWidgetResizable(true);
            scrollArea->setFrameShape(QFrame::NoFrame);

            m_stringPage = new QWidget();
            auto* l = new QVBoxLayout(m_stringPage);
            l->setContentsMargins(12, 12, 12, 12);
            l->setSpacing(6);

            // Key path label
            m_strKeyLabel = new QLabel(m_stringPage);
            m_strKeyLabel->setStyleSheet("color: #888; font-size: 10px;");
            m_strKeyLabel->setWordWrap(true);
            l->addWidget(m_strKeyLabel);

            auto* divider1 = new QFrame(m_stringPage);
            divider1->setFrameShape(QFrame::HLine);
            divider1->setFrameShadow(QFrame::Sunken);
            l->addWidget(divider1);

            // Reference section (read-only comparison)
            m_strRefHeader = new QLabel("Reference", m_stringPage);
            m_strRefHeader->setStyleSheet("font-weight: bold; font-size: 11px;");
            l->addWidget(m_strRefHeader);

            m_strRefEdit = new QPlainTextEdit(m_stringPage);
            m_strRefEdit->setReadOnly(true);
            m_strRefEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
            m_strRefEdit->setMaximumHeight(80);
            m_strRefEdit->setStyleSheet("background: #2a2a2a; color: #aaa;");
            l->addWidget(m_strRefEdit);

            // Working section (editable)
            m_strWorkHeader = new QLabel("Working", m_stringPage);
            m_strWorkHeader->setStyleSheet("font-weight: bold; font-size: 11px;");
            l->addWidget(m_strWorkHeader);

            m_strWorkEdit = new QPlainTextEdit(m_stringPage);
            m_strWorkEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
            m_strWorkEdit->setMinimumHeight(80);
            m_strWorkEdit->setPlaceholderText("(empty — type to set value)");
            connect(m_strWorkEdit, &QPlainTextEdit::textChanged,
                    this, &LexiconInspectorWidget::OnWorkingTextChanged);
            l->addWidget(m_strWorkEdit, 1);

            // Stats row
            m_strStats = new QLabel(m_stringPage);
            m_strStats->setStyleSheet("color: #666; font-size: 10px;");
            m_strStats->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            l->addWidget(m_strStats);

            // Browse Asset button
            m_browseAssetBtn = new QPushButton("Browse Asset\u2026", m_stringPage);
            m_browseAssetBtn->setToolTip(
                "Convert this entry to an asset reference.\n"
                "You will be prompted to enter the asset UUID.\n"
                "Full asset browser integration is planned for Phase 6.8.");
            connect(m_browseAssetBtn, &QPushButton::clicked,
                    this, &LexiconInspectorWidget::OnBrowseAsset);
            l->addWidget(m_browseAssetBtn, 0, Qt::AlignLeft);

            scrollArea->setWidget(m_stringPage);
            m_stack->addWidget(scrollArea); // index 2
        }

        // ---- Page 3: Asset entry -----------------------------------------
        {
            auto* scrollArea = new QScrollArea();
            scrollArea->setWidgetResizable(true);
            scrollArea->setFrameShape(QFrame::NoFrame);

            m_assetPage = new QWidget();
            auto* l = new QVBoxLayout(m_assetPage);
            l->setContentsMargins(12, 12, 12, 12);
            l->setSpacing(8);

            // Key path label
            m_assetKeyLabel = new QLabel(m_assetPage);
            m_assetKeyLabel->setStyleSheet("color: #888; font-size: 10px;");
            m_assetKeyLabel->setWordWrap(true);
            l->addWidget(m_assetKeyLabel);

            auto* divider2 = new QFrame(m_assetPage);
            divider2->setFrameShape(QFrame::HLine);
            divider2->setFrameShadow(QFrame::Sunken);
            l->addWidget(divider2);

            // Thumbnail
            m_assetThumb = new QLabel(m_assetPage);
            m_assetThumb->setFixedSize(ThumbSize, ThumbSize);
            m_assetThumb->setAlignment(Qt::AlignCenter);
            m_assetThumb->setStyleSheet(
                "background: #2a2a2a; border: 1px solid #444;"
                "color: #666; font-size: 28px;");
            l->addWidget(m_assetThumb, 0, Qt::AlignHCenter);

            // Type badge
            m_assetTypeLabel = new QLabel(m_assetPage);
            m_assetTypeLabel->setStyleSheet(
                "color: #aaa; font-size: 10px; font-style: italic;");
            m_assetTypeLabel->setAlignment(Qt::AlignCenter);
            l->addWidget(m_assetTypeLabel);

            // UUID
            auto* uuidHeader = new QLabel("UUID", m_assetPage);
            uuidHeader->setStyleSheet("font-weight: bold; font-size: 11px;");
            l->addWidget(uuidHeader);

            m_assetUuidLabel = new QLabel(m_assetPage);
            QFont mono = m_assetUuidLabel->font();
            mono.setFamily("Monospace");
            mono.setPointSize(9);
            m_assetUuidLabel->setFont(mono);
            m_assetUuidLabel->setWordWrap(true);
            m_assetUuidLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
            m_assetUuidLabel->setStyleSheet("color: #ccc;");
            l->addWidget(m_assetUuidLabel);

            // Asset path
            auto* pathHeader = new QLabel("Asset Path", m_assetPage);
            pathHeader->setStyleSheet("font-weight: bold; font-size: 11px;");
            l->addWidget(pathHeader);

            m_assetPathLabel = new QLabel(m_assetPage);
            m_assetPathLabel->setWordWrap(true);
            m_assetPathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
            m_assetPathLabel->setStyleSheet("color: #888; font-size: 10px;");
            l->addWidget(m_assetPathLabel);

            // Action buttons row
            auto* btnRow = new QHBoxLayout();

            m_changeAssetBtn = new QPushButton("Change Asset\u2026", m_assetPage);
            m_changeAssetBtn->setToolTip(
                "Pick a different asset to replace this UUID.\n"
                "Full asset browser integration is planned for Phase 6.8.");
            connect(m_changeAssetBtn, &QPushButton::clicked,
                    this, &LexiconInspectorWidget::OnBrowseAsset);
            btnRow->addWidget(m_changeAssetBtn);

            m_clearAssetBtn = new QPushButton("Clear", m_assetPage);
            m_clearAssetBtn->setToolTip(
                "Remove this asset reference and revert the entry to a plain string.");
            connect(m_clearAssetBtn, &QPushButton::clicked,
                    this, &LexiconInspectorWidget::OnClearAsset);
            btnRow->addWidget(m_clearAssetBtn);

            btnRow->addStretch(1);
            l->addLayout(btnRow);

            l->addStretch(1);
            scrollArea->setWidget(m_assetPage);
            m_stack->addWidget(scrollArea); // index 3
        }

        m_stack->setCurrentIndex(PageEmpty);
        setLayout(root);
    }

    ////////////////////////////////////////////////////////////////////////
    // Public — Show* methods

    void LexiconInspectorWidget::ShowStringEntry(const QString& key,
                                                  const QString& referenceValue,
                                                  const QString& workingValue)
    {
        m_currentKey = key;
        m_strKeyLabel->setText(key);

        const bool hasRef = !referenceValue.isEmpty();

        m_strRefHeader->setVisible(hasRef);
        m_strRefEdit->setVisible(hasRef);
        if (hasRef)
            m_strRefEdit->setPlainText(referenceValue);

        // Block signals while setting the value to avoid triggering a write-back
        // for a programmatic update.
        m_strWorkEdit->blockSignals(true);
        m_strWorkEdit->setPlainText(workingValue);
        m_strWorkEdit->blockSignals(false);

        UpdateStringStats(workingValue);
        m_stack->setCurrentIndex(PageString);
    }

    void LexiconInspectorWidget::ShowAssetEntry(const QString& key,
                                                 const QString& uuid,
                                                 const QString& resolvedPath,
                                                 const QString& absoluteSrcPath)
    {
        m_currentKey = key;
        m_assetKeyLabel->setText(key);
        m_assetUuidLabel->setText(uuid.isEmpty() ? "(no UUID)" : uuid);

        const QString ext    = QFileInfo(resolvedPath).suffix().toLower();
        const QString srcExt = QFileInfo(absoluteSrcPath).suffix().toLower();
        const bool isImage   = s_imageExtensions.contains(ext) || s_imageExtensions.contains(srcExt);
        const bool isAudio   = s_audioExtensions.contains(ext) || s_audioExtensions.contains(srcExt);

        if (isImage)
            m_assetTypeLabel->setText("Image Asset");
        else if (isAudio)
            m_assetTypeLabel->setText("Audio Asset");
        else if (!resolvedPath.isEmpty())
            m_assetTypeLabel->setText("Asset Reference");
        else
            m_assetTypeLabel->setText("Asset Reference (unresolved)");

        // Thumbnail
        bool thumbLoaded = false;
        if (isImage && !absoluteSrcPath.isEmpty())
        {
            QPixmap px(absoluteSrcPath);
            if (!px.isNull())
            {
                m_assetThumb->setPixmap(
                    px.scaled(ThumbSize, ThumbSize,
                              Qt::KeepAspectRatio, Qt::SmoothTransformation));
                thumbLoaded = true;
            }
        }
        if (!thumbLoaded)
        {
            if (isImage)       m_assetThumb->setText("\U0001F5BC");
            else if (isAudio)  m_assetThumb->setText("\U0001F3B5");
            else               m_assetThumb->setText("\U0001F4C4");
        }

        m_assetPathLabel->setText(
            resolvedPath.isEmpty()
                ? "(not resolved — ensure asset is processed)"
                : resolvedPath);

        m_stack->setCurrentIndex(PageAsset);
    }

    void LexiconInspectorWidget::ShowNamespace(const QString& path, int keyCount)
    {
        m_currentKey.clear();
        m_nsPathLabel->setText(path);
        m_nsCountLabel->setText(
            QString("%1 key%2 in this namespace")
                .arg(keyCount)
                .arg(keyCount == 1 ? "" : "s"));
        m_stack->setCurrentIndex(PageNamespace);
    }

    void LexiconInspectorWidget::ShowEmpty()
    {
        m_currentKey.clear();
        m_debounceTimer->stop();
        m_stack->setCurrentIndex(PageEmpty);
    }

    ////////////////////////////////////////////////////////////////////////
    // Private slots

    void LexiconInspectorWidget::OnWorkingTextChanged()
    {
        // Restart the debounce timer on every keystroke.
        m_debounceTimer->start();
        UpdateStringStats(m_strWorkEdit->toPlainText());
    }

    void LexiconInspectorWidget::OnDebounceTimeout()
    {
        if (m_currentKey.isEmpty())
            return;
        emit ValueEdited(m_currentKey, m_strWorkEdit->toPlainText());
    }

    void LexiconInspectorWidget::OnBrowseAsset()
    {
        if (m_currentKey.isEmpty())
            return;
        emit BrowseAssetRequested(m_currentKey);
    }

    void LexiconInspectorWidget::OnClearAsset()
    {
        if (m_currentKey.isEmpty())
            return;
        // Emit an empty string — the tool window writes "" (plain string) to the file,
        // clearing the asset reference and switching the entry back to text mode.
        emit ValueEdited(m_currentKey, QString{});
    }

    ////////////////////////////////////////////////////////////////////////
    // Private helpers

    void LexiconInspectorWidget::UpdateStringStats(const QString& text)
    {
        const int charCount = text.length();
        const QStringList words = text.split(
            QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        const int wordCount = words.size();

        m_strStats->setText(
            QString("%1 char%2 · %3 word%4")
                .arg(charCount).arg(charCount == 1 ? "" : "s")
                .arg(wordCount).arg(wordCount == 1 ? "" : "s"));
    }

    ////////////////////////////////////////////////////////////////////////

} // namespace FoundationLocalisation

#include <moc_LexiconInspectorWidget.cpp>
