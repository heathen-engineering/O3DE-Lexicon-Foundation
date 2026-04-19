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
#include <QVector>

namespace FoundationLocalisation
{
    ///<summary>
    /// QAbstractTableModel for the Workbench (translation grid).
    ///
    /// Columns:
    ///   0 — Status  (Unicode glyph: ✓ translated · ⚠ missing · ✗ orphan · ○ empty)
    ///   1 — Key     (full dot-path; read-only)
    ///   2 — Source  (value from reference culture; read-only)
    ///   3 — Active  (value from editable culture; inline-editable)
    ///
    /// SetEntries() populates the model from the shared LexiconEntryMap, applying
    /// an optional key-prefix filter so the table mirrors the Explorer tree selection.
    ///
    /// When the user commits an edit in the Active column, ActiveValueEdited() is
    /// emitted. LexiconToolWindow handles the signal and writes the change back to
    /// the active .helex file, then refreshes both models.
    ///</summary>
    class LexiconTableModel : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        /// Column index constants — used by LexiconToolWindow when configuring the view.
        enum Column { ColStatus = 0, ColKey, ColSource, ColActive, ColCount };

        explicit LexiconTableModel(QObject* parent = nullptr);

        ///<summary>
        /// Rebuilds the visible row set from the given map.
        ///
        /// filterPrefix — show only keys that equal the prefix or start with
        ///                "prefix.". Pass empty to show all entries.
        /// hasSourceFile / hasActiveFile — used to determine whether an empty
        ///                value indicates a missing translation or a file simply
        ///                not being open yet.
        ///</summary>
        void SetEntries(const LexiconEntryMap& entries,
                        const QString&          filterPrefix,
                        bool                    hasSourceFile,
                        bool                    hasActiveFile);

        // QAbstractTableModel
        int           rowCount(const QModelIndex& parent = {})    const override;
        int           columnCount(const QModelIndex& parent = {}) const override;
        QVariant      data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant      headerData(int section, Qt::Orientation orientation,
                                 int role = Qt::DisplayRole) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        bool          setData(const QModelIndex& index, const QVariant& value,
                              int role = Qt::EditRole) override;

    signals:
        ///<summary>
        /// Fired when the user commits a change to the Active Value column.
        /// LexiconToolWindow connects this to its file write-back slot.
        ///</summary>
        void ActiveValueEdited(const QString& key, const QString& newValue);

    private:
        struct Row
        {
            QString      key;
            LexiconEntry entry;
        };

        static QString StatusGlyph(const LexiconEntry& e, bool hasSource, bool hasActive);
        static QColor  StatusColor(const LexiconEntry& e, bool hasSource, bool hasActive);

        QVector<Row> m_rows;
        bool m_hasSourceFile = false;
        bool m_hasActiveFile = false;
    };

} // namespace FoundationLocalisation
