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

    void LexiconTableModel::SetDefaultEntries(const LexiconEntryMap& entries,
                                               const QString& filterPrefix)
    {
        beginResetModel();
        m_rows.clear();

        for (auto it = entries.begin(); it != entries.end(); ++it)
        {
            const QString& key = it.key();
            if (!filterPrefix.isEmpty() &&
                key != filterPrefix && !key.startsWith(filterPrefix + QLatin1Char('.')))
                continue;
            m_rows.append(Row{ key, it.value() });
        }

        endResetModel();
    }

    void LexiconTableModel::SetExtraColumns(const QVector<ExtraColumn>& extraCols)
    {
        beginResetModel();
        m_extraCols = extraCols;
        endResetModel();
    }

    void LexiconTableModel::SetShowAddColumn(bool show)
    {
        if (m_showAddColumn == show) return;
        beginResetModel();
        m_showAddColumn = show;
        endResetModel();
    }

    // ── QAbstractTableModel ───────────────────────────────────────────────────

    int LexiconTableModel::rowCount(const QModelIndex& parent) const
    {
        return parent.isValid() ? 0 : m_rows.size();
    }

    int LexiconTableModel::columnCount(const QModelIndex& parent) const
    {
        if (parent.isValid()) return 0;
        return FixedColCount + m_extraCols.size() + (m_showAddColumn ? 1 : 0);
    }

    QVariant LexiconTableModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid() || index.row() >= m_rows.size())
            return {};

        const Row& row = m_rows[index.row()];
        const int  col = index.column();
        const bool isExtra = (col >= FixedColCount);
        const int  extraIdx = col - FixedColCount;

        // Phantom [+] column — no data in cells, only in header
        if (m_showAddColumn && col == FixedColCount + (int)m_extraCols.size())
            return {};

        switch (role)
        {
        case Qt::DisplayRole:
            if (col == ColKey)     return row.key;
            if (col == ColType)    return HintTypeLabel(row.entry.hintType);
            if (col == ColDefault) return row.entry.activeValue;
            if (isExtra && extraIdx < m_extraCols.size())
                return m_extraCols[extraIdx].values.value(row.key, QString{});
            return {};

        case Qt::EditRole:
            if (col == ColType)    return static_cast<int>(row.entry.hintType);
            if (col == ColDefault) return row.entry.activeValue;
            if (isExtra && extraIdx < m_extraCols.size())
                return m_extraCols[extraIdx].values.value(row.key, QString{});
            return {};

        case Qt::ForegroundRole:
            return {};

        case Qt::BackgroundRole:
        {
            // Grey = empty default; amber = empty extra column
            if (col == ColDefault && row.entry.activeValue.isEmpty())
                return QColor(60, 60, 60);
            if (isExtra && extraIdx < m_extraCols.size())
            {
                const QString v = m_extraCols[extraIdx].values.value(row.key, QString{});
                if (v.isEmpty())
                    return QColor(80, 70, 30);
            }
            return {};
        }

        case Qt::TextAlignmentRole:
            return {};

        case Qt::ToolTipRole:
            if (col == ColKey) return row.key;
            return {};

        default:
            return {};
        }
    }

    QVariant LexiconTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
            return {};

        if (section == ColKey)     return {}; // header replaced by the key-input overlay widget
        if (section == ColType)    return QStringLiteral("Type");
        if (section == ColDefault) return QStringLiteral("Default");

        const int extraIdx = section - FixedColCount;
        if (extraIdx >= 0 && extraIdx < m_extraCols.size())
            return m_extraCols[extraIdx].displayName;

        // Phantom [+] add-column button in the header
        if (m_showAddColumn && section == FixedColCount + (int)m_extraCols.size())
            return QStringLiteral("[+]");

        return {};
    }

    Qt::ItemFlags LexiconTableModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
            return Qt::NoItemFlags;

        const int col = index.column();

        // Phantom [+] column — not interactive in cells
        if (m_showAddColumn && col == FixedColCount + (int)m_extraCols.size())
            return Qt::NoItemFlags;

        Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        if (col == ColType || col == ColDefault || col >= FixedColCount)
            f |= Qt::ItemIsEditable;

        return f;
    }

    bool LexiconTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
    {
        if (!index.isValid() || index.row() >= m_rows.size())
            return false;

        if (role != Qt::EditRole)
            return false;

        Row& row = m_rows[index.row()];
        const int col = index.column();

        if (col == ColType)
        {
            const auto hint = static_cast<LexiconHintType>(value.toInt());
            if (row.entry.hintType == hint) return false;
            row.entry.hintType = hint;
            row.entry.isAsset  = (hint != LexiconHintType::None && hint != LexiconHintType::String);
            emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
            emit TypeChanged(row.key, hint);
            return true;
        }

        if (col == ColDefault)
        {
            const QString newVal = value.toString();
            if (row.entry.activeValue == newVal) return false;
            row.entry.activeValue = newVal;
            emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
            emit DefaultValueEdited(row.key, newVal);
            return true;
        }

        const int extraIdx = col - FixedColCount;
        if (extraIdx >= 0 && extraIdx < m_extraCols.size())
        {
            const QString newVal = value.toString();
            m_extraCols[extraIdx].values[row.key] = newVal;
            emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole });
            emit ExtraValueEdited(row.key, extraIdx, newVal);
            return true;
        }

        return false;
    }

    // ── Hint type helpers ─────────────────────────────────────────────────────

    QString LexiconTableModel::HintTypeLabel(LexiconHintType hint)
    {
        switch (hint)
        {
        case LexiconHintType::String:    return QStringLiteral("String");
        case LexiconHintType::Sound:     return QStringLiteral("Sound");
        case LexiconHintType::Texture:   return QStringLiteral("Texture");
        case LexiconHintType::Spawnable: return QStringLiteral("Spawnable");
        case LexiconHintType::Asset:     return QStringLiteral("Asset");
        default:                         return QStringLiteral("String");
        }
    }

    QStringList LexiconTableModel::HintTypeLabels()
    {
        return {
            QStringLiteral("String"),
            QStringLiteral("Sound"),
            QStringLiteral("Texture"),
            QStringLiteral("Spawnable"),
            QStringLiteral("Asset"),
        };
    }

    LexiconHintType LexiconTableModel::HintTypeFromIndex(int idx)
    {
        switch (idx)
        {
        case 0: return LexiconHintType::String;
        case 1: return LexiconHintType::Sound;
        case 2: return LexiconHintType::Texture;
        case 3: return LexiconHintType::Spawnable;
        case 4: return LexiconHintType::Asset;
        default: return LexiconHintType::String;
        }
    }

    ////////////////////////////////////////////////////////////////////////

} // namespace FoundationLocalisation

#include <moc_LexiconTableModel.cpp>
