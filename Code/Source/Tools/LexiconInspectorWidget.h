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

#include <QString>
#include <QWidget>

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QTimer;

namespace FoundationLocalisation
{
    ///<summary>
    /// Inspector panel shown on the right side of the Lexicon Tool.
    ///
    /// Displays contextual detail for the currently-selected Explorer tree node
    /// and provides editing for the working value.
    ///
    ///   Leaf — string key:
    ///     Reference value (read-only, greyed, hidden when no reference file is open).
    ///     Working value — fully editable QPlainTextEdit.
    ///     Character and word count.
    ///     "Browse Asset…" button to convert the entry to an asset reference.
    ///
    ///   Leaf — asset-reference key:
    ///     Asset UUID in monospace (selectable for copy).
    ///     Resolved relative path from the asset catalog (when available).
    ///     Image thumbnail if the asset is a recognisable image format.
    ///     "Change Asset…" and "Clear" buttons.
    ///
    ///   Namespace node (intermediate, no own value):
    ///     Full dot-path and count of leaf keys in the subtree.
    ///
    ///   Nothing selected:
    ///     Placeholder label.
    ///
    /// Editing is debounced (500 ms). ValueEdited is emitted after the user
    /// stops typing. BrowseAssetRequested is emitted when the user clicks
    /// Browse/Change Asset — LexiconToolWindow handles the actual file write.
    ///
    /// This widget is pure Qt — no O3DE headers.
    ///</summary>
    class LexiconInspectorWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit LexiconInspectorWidget(QWidget* parent = nullptr);

        ///<summary>
        /// Show a string-type leaf entry.
        /// referenceValue may be empty (no reference file open).
        /// workingValue may be empty (key not yet translated).
        ///</summary>
        void ShowStringEntry(const QString& key,
                             const QString& referenceValue,
                             const QString& workingValue);

        ///<summary>
        /// Show an asset-reference leaf entry.
        /// resolvedPath and absoluteSrcPath may be empty if the catalog lookup
        /// has not been wired up yet (Phase 6.8).
        ///</summary>
        void ShowAssetEntry(const QString& key,
                            const QString& uuid,
                            const QString& resolvedPath,
                            const QString& absoluteSrcPath);

        ///<summary>Show a namespace (intermediate) node.</summary>
        void ShowNamespace(const QString& path, int keyCount);

        ///<summary>Show the empty / no-selection placeholder.</summary>
        void ShowEmpty();

    signals:
        ///<summary>
        /// Emitted ~500 ms after the user stops editing the working value.
        /// LexiconToolWindow connects this to its file write-back.
        ///</summary>
        void ValueEdited(const QString& key, const QString& value);

        ///<summary>
        /// Emitted when the user clicks Browse/Change Asset.
        /// LexiconToolWindow opens a UUID input dialog (or asset browser in Phase 6.8)
        /// and writes the result back via WriteValueToFile.
        ///</summary>
        void BrowseAssetRequested(const QString& key);

    private slots:
        void OnWorkingTextChanged();
        void OnDebounceTimeout();
        void OnBrowseAsset();
        void OnClearAsset();

    private:
        void UpdateStringStats(const QString& text);

        QStackedWidget* m_stack = nullptr;
        QTimer*         m_debounceTimer = nullptr;
        QString         m_currentKey;              ///< key currently displayed

        enum Page
        {
            PageEmpty     = 0,
            PageNamespace = 1,
            PageString    = 2,
            PageAsset     = 3,
        };

        // ---- Empty page ----
        QWidget* m_emptyPage = nullptr;

        // ---- Namespace page ----
        QWidget* m_namespacePage  = nullptr;
        QLabel*  m_nsPathLabel    = nullptr;
        QLabel*  m_nsCountLabel   = nullptr;

        // ---- String page ----
        QWidget*        m_stringPage       = nullptr;
        QLabel*         m_strKeyLabel      = nullptr;
        QLabel*         m_strRefHeader     = nullptr;
        QPlainTextEdit* m_strRefEdit       = nullptr;   ///< reference value (always read-only)
        QLabel*         m_strWorkHeader    = nullptr;
        QPlainTextEdit* m_strWorkEdit      = nullptr;   ///< working value (editable)
        QLabel*         m_strStats         = nullptr;   ///< char / word count
        QPushButton*    m_browseAssetBtn   = nullptr;   ///< convert to asset reference

        // ---- Asset page ----
        QWidget*     m_assetPage      = nullptr;
        QLabel*      m_assetKeyLabel  = nullptr;
        QLabel*      m_assetThumb     = nullptr;
        QLabel*      m_assetTypeLabel = nullptr;
        QLabel*      m_assetUuidLabel = nullptr;
        QLabel*      m_assetPathLabel = nullptr;
        QPushButton* m_changeAssetBtn = nullptr;
        QPushButton* m_clearAssetBtn  = nullptr;
    };

} // namespace FoundationLocalisation
