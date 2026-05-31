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

#include <QAbstractTableModel>
#include <QMap>
#include <QVector>

namespace FoundationLocalisation
{
    ///<summary>
    /// QAbstractTableModel for the Lexicon Workbench.
    ///
    /// Columns (matching Unity's Localisation Lexicon Project Settings):
    ///   0 — Delete   (✕ — click to remove key from all files; handled via DeleteKeyRequested signal)
    ///   1 — Key      (full dot-path; read-only)
    ///   2 — Type     (LexiconHintType dropdown: String/Sound/Texture/Spawnable/Asset)
    ///   3 — Default  (value in the designated default .helex; inline-editable)
    ///   4+ — Extra   (one column per additional culture file; headers show assetId)
    ///
    /// Empty Default cells are highlighted grey; empty Extra cells are highlighted amber.
    ///</summary>
    class LexiconTableModel : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        enum FixedColumn { ColKey = 0, ColType, ColDefault, FixedColCount };

        // Extra culture file column descriptor
        struct ExtraColumn
        {
            QString filePath;     ///< absolute path to the .helex file
            QString displayName;  ///< assetId shown in the column header
            QMap<QString, QString> values; ///< key → value in this file
        };

        explicit LexiconTableModel(QObject* parent = nullptr);

        /// Populate or refresh the model.
        void SetDefaultEntries(const LexiconEntryMap& entries,
                               const QString&          filterPrefix = {});

        /// Replace the list of extra culture columns. Pass an empty vector to clear.
        void SetExtraColumns(const QVector<ExtraColumn>& extraCols);

        /// When true, an extra "[+]" column appears at the far right of the header.
        /// Clicking it (via QHeaderView::sectionClicked) should trigger ShowExtraColumnPicker().
        /// Only set this when there are additional .helex files the user can add.
        void SetShowAddColumn(bool show);
        bool HasAddColumnButton() const { return m_showAddColumn; }

        // QAbstractTableModel
        int           rowCount   (const QModelIndex& parent = {}) const override;
        int           columnCount(const QModelIndex& parent = {}) const override;
        QVariant      data       (const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant      headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
        Qt::ItemFlags flags      (const QModelIndex& index) const override;
        bool          setData    (const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

        /// Hint type display strings and round-trip helpers (used by the Type delegate too)
        static QString HintTypeLabel(LexiconHintType hint);
        static QStringList HintTypeLabels();     ///< all labels in enum order, excluding None
        static LexiconHintType HintTypeFromIndex(int idx); ///< 0-based excluding None

    signals:
        void TypeChanged(const QString& key, LexiconHintType newHint);
        void DefaultValueEdited(const QString& key, const QString& newValue);
        void ExtraValueEdited(const QString& key, int extraColIndex, const QString& newValue);

    private:
        struct Row
        {
            QString      key;
            LexiconEntry entry;
        };

        QVector<Row>         m_rows;
        QVector<ExtraColumn> m_extraCols;
        bool                 m_showAddColumn = false;
    };

} // namespace FoundationLocalisation
