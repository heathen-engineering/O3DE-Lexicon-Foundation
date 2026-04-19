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

#include "LexiconTreeModel.h"
#include "LexiconTreeItem.h"

#include <QColor>
#include <functional>
#include <QFile>
#include <QFont>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>

namespace FoundationLocalisation
{
    ////////////////////////////////////////////////////////////////////////
    // LexiconTreeItem implementation

    LexiconTreeItem::LexiconTreeItem(const QString& segment,
                                      const QString& fullPath,
                                      LexiconTreeItem* parent)
        : m_segment(segment)
        , m_fullPath(fullPath)
        , m_parent(parent)
    {
    }

    void LexiconTreeItem::AddChild(std::unique_ptr<LexiconTreeItem> child)
    {
        m_children.push_back(std::move(child));
    }

    LexiconTreeItem* LexiconTreeItem::FindChild(const QString& segment) const
    {
        for (const auto& child : m_children)
        {
            if (child->m_segment == segment)
                return child.get();
        }
        return nullptr;
    }

    LexiconTreeItem* LexiconTreeItem::Child(int row) const
    {
        if (row < 0 || row >= static_cast<int>(m_children.size()))
            return nullptr;
        return m_children[static_cast<size_t>(row)].get();
    }

    int LexiconTreeItem::Row() const
    {
        if (!m_parent)
            return 0;

        const auto& siblings = m_parent->m_children;
        for (int i = 0; i < static_cast<int>(siblings.size()); ++i)
        {
            if (siblings[static_cast<size_t>(i)].get() == this)
                return i;
        }
        return 0;
    }

    ////////////////////////////////////////////////////////////////////////
    // LexiconTreeModel

    LexiconTreeModel::LexiconTreeModel(QObject* parent)
        : QAbstractItemModel(parent)
        , m_root(std::make_unique<LexiconTreeItem>(QString{}, QString{}, nullptr))
    {
    }

    LexiconTreeModel::~LexiconTreeModel() = default;

    void LexiconTreeModel::LoadFiles(const QString& sourcePath, const QString& activePath)
    {
        m_sourcePath = sourcePath;
        m_activePath = activePath;
        m_entries.clear();

        if (!sourcePath.isEmpty())
            ParseHelexFile(sourcePath, m_entries, /*isSource=*/true);

        if (!activePath.isEmpty())
            ParseHelexFile(activePath, m_entries, /*isSource=*/false);

        RebuildTree();
    }

    QString LexiconTreeModel::FullPathForIndex(const QModelIndex& index) const
    {
        if (!index.isValid())
            return {};
        const auto* item = static_cast<const LexiconTreeItem*>(index.internalPointer());
        return item ? item->FullPath() : QString{};
    }

    bool LexiconTreeModel::IsLeafIndex(const QModelIndex& index) const
    {
        if (!index.isValid())
            return false;
        const auto* item = static_cast<const LexiconTreeItem*>(index.internalPointer());
        return item && item->IsLeaf();
    }

    ////////////////////////////////////////////////////////////////////////
    // QAbstractItemModel

    QModelIndex LexiconTreeModel::index(int row, int column,
                                         const QModelIndex& parent) const
    {
        if (!hasIndex(row, column, parent))
            return {};

        const LexiconTreeItem* parentItem = parent.isValid()
            ? static_cast<const LexiconTreeItem*>(parent.internalPointer())
            : m_root.get();

        LexiconTreeItem* child = parentItem->Child(row);
        return child ? createIndex(row, column, child) : QModelIndex{};
    }

    QModelIndex LexiconTreeModel::parent(const QModelIndex& child) const
    {
        if (!child.isValid())
            return {};

        const auto* childItem  = static_cast<const LexiconTreeItem*>(child.internalPointer());
        LexiconTreeItem* parentItem = childItem->Parent();

        if (!parentItem || parentItem == m_root.get())
            return {};

        return createIndex(parentItem->Row(), 0, parentItem);
    }

    int LexiconTreeModel::rowCount(const QModelIndex& parent) const
    {
        const LexiconTreeItem* item = parent.isValid()
            ? static_cast<const LexiconTreeItem*>(parent.internalPointer())
            : m_root.get();
        return item ? item->ChildCount() : 0;
    }

    int LexiconTreeModel::columnCount([[maybe_unused]] const QModelIndex& parent) const
    {
        return 1; // Key segment name only; source/active values are in the Workbench
    }

    QVariant LexiconTreeModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
            return {};

        const auto* item = static_cast<const LexiconTreeItem*>(index.internalPointer());
        if (!item)
            return {};

        switch (role)
        {
        case Qt::DisplayRole:
            return item->Segment();

        case Qt::ToolTipRole:
            // Full dot-path on hover
            return item->FullPath();

        case Qt::FontRole:
        {
            QFont f;
            // Intermediate nodes (namespace groupings) are displayed bold
            if (!item->IsLeaf())
                f.setBold(true);
            return f;
        }

        case Qt::ForegroundRole:
            if (item->IsLeaf())
            {
                // Flags are set by LexiconValidator (Phase 6.6) — look up from entry map
                // so the colour reflects the most recent validation pass.
                auto entryIt = m_entries.find(item->FullPath());
                if (entryIt != m_entries.end())
                {
                    const LexiconEntry& e = entryIt.value();
                    if (e.isMissing)   return QColor(200, 160,   0); // amber  — needs translation
                    if (e.isOrphan)    return QColor(120, 120, 120); // grey   — orphan key
                    if (e.isDuplicate) return QColor(160, 100, 200); // purple — duplicate value
                    if (e.isEmpty)     return QColor(150, 150, 150); // light grey — untouched
                    // Leaf with content and no issues
                    if (!e.activeValue.isEmpty())
                        return QColor(80, 200, 80);                  // green  — translated
                }
            }
            else if (item->hasIssues)
            {
                // Namespace node: amber badge if any descendant has an issue
                return QColor(200, 160, 0);
            }
            break;

        default:
            break;
        }

        return {};
    }

    QVariant LexiconTreeModel::headerData(int section,
                                           Qt::Orientation orientation,
                                           int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole && section == 0)
            return QStringLiteral("Key");
        return {};
    }

    Qt::ItemFlags LexiconTreeModel::flags(const QModelIndex& index) const
    {
        if (!index.isValid())
            return Qt::NoItemFlags;
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    ////////////////////////////////////////////////////////////////////////
    // Private helpers

    void LexiconTreeModel::ApplyValidationFlags(const LexiconEntryMap& validated)
    {
        // Copy flags from the validated map into m_entries
        for (auto it = validated.cbegin(); it != validated.cend(); ++it)
        {
            auto existing = m_entries.find(it.key());
            if (existing != m_entries.end())
            {
                existing->isMissing   = it->isMissing;
                existing->isOrphan    = it->isOrphan;
                existing->isDuplicate = it->isDuplicate;
                existing->isEmpty     = it->isEmpty;
            }
        }

        // Propagate hasIssues badges bottom-up through the tree so namespace nodes
        // reflect the worst state of any descendant leaf.
        std::function<bool(LexiconTreeItem*)> propagate = [&](LexiconTreeItem* node) -> bool
        {
            bool anyIssue = false;

            if (node->IsLeaf())
            {
                auto it = m_entries.find(node->FullPath());
                if (it != m_entries.end())
                {
                    const LexiconEntry& e = it.value();
                    anyIssue = e.isMissing || e.isOrphan || e.isDuplicate || e.isEmpty;
                }
            }

            for (int i = 0; i < node->ChildCount(); ++i)
                anyIssue |= propagate(node->Child(i));

            node->hasIssues = anyIssue;
            return anyIssue;
        };

        propagate(m_root.get());

        // Trigger a full visual refresh without rebuilding the tree structure
        beginResetModel();
        endResetModel();
    }

    void LexiconTreeModel::RebuildTree()
    {
        beginResetModel();
        m_root = std::make_unique<LexiconTreeItem>(QString{}, QString{}, nullptr);

        for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
        {
            const QStringList segments = it.key().split(QLatin1Char('.'), Qt::SkipEmptyParts);
            if (segments.isEmpty())
                continue;

            LexiconTreeItem* current = m_root.get();
            QString fullPath;

            for (int i = 0; i < segments.size(); ++i)
            {
                if (!fullPath.isEmpty())
                    fullPath += QLatin1Char('.');
                fullPath += segments[i];

                LexiconTreeItem* child = current->FindChild(segments[i]);
                if (!child)
                {
                    auto newNode = std::make_unique<LexiconTreeItem>(
                        segments[i], fullPath, current);
                    child = newNode.get();
                    current->AddChild(std::move(newNode));
                }

                // Mark the terminal segment as a leaf and copy its data
                if (i == segments.size() - 1)
                {
                    child->MarkAsLeaf();
                    child->sourceValue = it.value().sourceValue;
                    child->activeValue = it.value().activeValue;
                    child->isAsset     = it.value().isAsset;
                }

                current = child;
            }
        }

        endResetModel();
        emit FilesLoaded(static_cast<int>(m_entries.size()));
    }

    bool LexiconTreeModel::ParseHelexFile(const QString& path,
                                           LexiconEntryMap& out,
                                           bool isSource)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return false;

        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);

        if (err.error != QJsonParseError::NoError || !doc.isObject())
            return false;

        const QJsonObject entries = doc.object().value(QLatin1String("entries")).toObject();

        for (auto it = entries.begin(); it != entries.end(); ++it)
        {
            const QString key = it.key();
            LexiconEntry& entry = out[key];

            const QJsonValue val = it.value();

            if (val.isString())
            {
                if (isSource)
                    entry.sourceValue = val.toString();
                else
                    entry.activeValue = val.toString();
                entry.isAsset = false;
            }
            else if (val.isObject())
            {
                const QString uuid = val.toObject().value(QLatin1String("uuid")).toString();
                if (!uuid.isEmpty())
                {
                    if (isSource)
                        entry.sourceValue = uuid;
                    else
                        entry.activeValue = uuid;
                    entry.isAsset = true;
                }
            }
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////

} // namespace FoundationLocalisation

#include <moc_LexiconTreeModel.cpp>
