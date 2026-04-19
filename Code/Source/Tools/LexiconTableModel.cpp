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

#include "LexiconTableModel.h"

#include <QColor>

namespace FoundationLocalisation
{
    LexiconTableModel::LexiconTableModel(QObject* parent)
        : QAbstractTableModel(parent)
    {
    }

    void LexiconTableModel::SetEntries(const LexiconEntryMap& entries,
                                        const QString&          filterPrefix,
                                        bool                    hasSourceFile,
                                        bool                    hasActiveFile)
    {
        beginResetModel();

        m_hasSourceFile = hasSourceFile;
        m_hasActiveFile = hasActiveFile;
        m_rows.clear();

        for (auto it = entries.begin(); it != entries.end(); ++it)
        {
            const QString& key = it.key();

            // Apply prefix filter: exact match or child key
            if (!filterPrefix.isEmpty())
            {
                if (key != filterPrefix && !key.startsWith(filterPrefix + QLatin1Char('.')))
                    continue;
            }

            m_rows.append(Row{ key, it.value() });
        }

        endResetModel();
    }

    ////////////////////////////////////////////////////////////////////////
    // QAbstractTableModel

    int LexiconTableModel::rowCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : m_rows.size();
    }

    int LexiconTableModel::columnCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : ColCount;
    }

    QVariant LexiconTableModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid() || index.row() >= m_rows.size())
            return {};

        const Row& row = m_rows[index.row()];
        const int  col = index.column();

        switch (role)
        {
        case Qt::DisplayRole:
            switch (col)
            {
            case ColStatus: return StatusGlyph(row.entry, m_hasSourceFile, m_hasActiveFile);
            case ColKey:    return row.key;
            case ColSource: return row.entry.sourceValue;
            case ColActive: return row.entry.activeValue;
            default:        return {};
            }

        case Qt::ForegroundRole:
            if (col == ColStatus)
                return StatusColor(row.entry, m_hasSourceFile, m_hasActiveFile);
            return {};

        case Qt::ToolTipRole:
            if (col == ColStatus)
            {
                const LexiconEntry& e = row.entry;
                if (!m_hasSourceFile && !m_hasActiveFile)  return QStringLiteral("No files open");
                if (e.sourceValue.isEmpty() && e.activeValue.isEmpty())  return QStringLiteral("Empty — no value in either file");
                if (!m_hasActiveFile || e.activeValue.isEmpty())         return QStringLiteral("Missing translation in active file");
                if (!m_hasSourceFile || e.sourceValue.isEmpty())         return QStringLiteral("Orphan — key not in source file");
                return QStringLiteral("Translated");
            }
            if (col == ColKey) return row.key;
            return {};

        case Qt::EditRole:
            if (col == ColActive) return row.entry.activeValue;
            return {};

        default:
            return {};
        }
    }

    QVariant LexiconTableModel::headerData(int section,
                                            Qt::Orientation orientation,
                                            int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return {};

        switch (section)
        {
        case ColStatus: return QStringLiteral("St.");
        case ColKey:    return QStringLiteral("Key");
        case ColSource: return QStringLiteral("Source Value");
        case ColActive: return QStringLiteral("Active Value");
        default:        return {};
        }
    }

    Qt::ItemFlags LexiconTableModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
            return Qt::NoItemFlags;

        Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (index.column() == ColActive && m_hasActiveFile)
            f |= Qt::ItemIsEditable;
        return f;
    }

    bool LexiconTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (role != Qt::EditRole || !index.isValid())
            return false;
        if (index.column() != ColActive || index.row() >= m_rows.size())
            return false;

        const QString newValue = value.toString();
        Row& row = m_rows[index.row()];

        if (row.entry.activeValue == newValue)
            return false;

        row.entry.activeValue = newValue;
        emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
        emit ActiveValueEdited(row.key, newValue);
        return true;
    }

    ////////////////////////////////////////////////////////////////////////
    // Private helpers

    QString LexiconTableModel::StatusGlyph(const LexiconEntry& e,
                                            bool hasSource,
                                            bool hasActive)
    {
        if (!hasSource && !hasActive)                                      return QStringLiteral("\u25CB"); // ○
        if (hasActive && hasSource && !e.activeValue.isEmpty())            return QStringLiteral("\u2713"); // ✓
        if (hasActive && (e.activeValue.isEmpty()) && !e.sourceValue.isEmpty()) return QStringLiteral("\u26A0"); // ⚠
        if (hasSource && e.sourceValue.isEmpty() && !e.activeValue.isEmpty())   return QStringLiteral("\u2717"); // ✗
        return QStringLiteral("\u25CB");                                        // ○ fallback
    }

    QColor LexiconTableModel::StatusColor(const LexiconEntry& e,
                                           bool hasSource,
                                           bool hasActive)
    {
        if (!hasSource && !hasActive)                                           return QColor(120, 120, 120);
        if (hasActive && hasSource && !e.activeValue.isEmpty())                 return QColor(80,  200, 80);  // green
        if (hasActive && e.activeValue.isEmpty() && !e.sourceValue.isEmpty())   return QColor(200, 160, 0);   // amber
        if (hasSource && e.sourceValue.isEmpty() && !e.activeValue.isEmpty())   return QColor(160, 80,  80);  // red/grey
        return QColor(120, 120, 120);
    }

    ////////////////////////////////////////////////////////////////////////

} // namespace FoundationLocalisation

#include <moc_LexiconTableModel.cpp>
