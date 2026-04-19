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

#include <QAbstractItemModel>

#include <memory>

namespace FoundationLocalisation
{
    class LexiconTreeItem;

    ///<summary>
    /// QAbstractItemModel that presents loaded .helex keys as a dot-tokenised tree.
    ///
    /// The Explorer QTreeView uses this model. It exposes one column (the key segment
    /// name) with intermediate nodes in bold and leaf nodes coloured to indicate
    /// translation status once an active culture file is loaded.
    ///
    /// Parsing is done with QJsonDocument (pure Qt, no AZ/rapidjson dependency in
    /// this class) to keep the model decoupled from the O3DE build infrastructure.
    ///
    /// The full flat entry map (LexiconEntryMap) is also accessible to the
    /// LexiconTableModel (Workbench, Phase 6.3) so both views share one parse.
    ///</summary>
    class LexiconTreeModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        explicit LexiconTreeModel(QObject* parent = nullptr);
        ~LexiconTreeModel() override;

        ///<summary>
        /// Parses (or re-parses) the two .helex files and rebuilds the tree.
        /// Either path may be empty — the model handles partial loads gracefully.
        ///</summary>
        void LoadFiles(const QString& sourcePath, const QString& activePath);

        ///<summary>Returns the shared flat entry map for use by LexiconTableModel.</summary>
        const LexiconEntryMap& EntryMap() const { return m_entries; }

        ///<summary>
        /// Applies validated flags from a post-Validate() entry map back into the
        /// tree's own m_entries and propagates the hasIssues badge up the tree
        /// hierarchy so namespace nodes reflect whether any descendant has issues.
        ///
        /// Call this after LexiconValidator::Validate() has been run on a copy of
        /// EntryMap(). Triggers a model-wide data change notification to refresh
        /// ForegroundRole colours without rebuilding the tree structure.
        ///</summary>
        void ApplyValidationFlags(const LexiconEntryMap& validated);

        ///<summary>Returns the full dot-path for the given model index, or empty if invalid.</summary>
        QString FullPathForIndex(const QModelIndex& index) const;

        ///<summary>
        /// Returns true when the item at index represents a key that actually exists in
        /// one of the open .helex files — i.e. LexiconTreeItem::IsLeaf() is true.
        /// A node can be both a leaf AND have children (e.g. "Menu" and "Menu.Play" both
        /// exist as keys). Use this instead of rowCount()==0 to determine leaf status.
        ///</summary>
        bool IsLeafIndex(const QModelIndex& index) const;

        // ---- QAbstractItemModel interface -----------------------------------

        QModelIndex   index(int row, int column,
                            const QModelIndex& parent = {}) const override;
        QModelIndex   parent(const QModelIndex& child)    const override;
        int           rowCount(const QModelIndex& parent = {})   const override;
        int           columnCount(const QModelIndex& parent = {}) const override;
        QVariant      data(const QModelIndex& index,
                           int role = Qt::DisplayRole)    const override;
        QVariant      headerData(int section, Qt::Orientation orientation,
                                 int role = Qt::DisplayRole) const override;
        Qt::ItemFlags flags(const QModelIndex& index)    const override;

    signals:
        ///<summary>Emitted after a successful LoadFiles() with the total key count.</summary>
        void FilesLoaded(int keyCount);

    private:
        void RebuildTree();

        ///<summary>
        /// Parses a single .helex file into the given map.
        /// isSource controls which field (sourceValue / activeValue) receives values.
        ///</summary>
        static bool ParseHelexFile(const QString& path,
                                   LexiconEntryMap& out,
                                   bool isSource);

        std::unique_ptr<LexiconTreeItem> m_root;
        LexiconEntryMap                  m_entries;

        QString m_sourcePath;
        QString m_activePath;
    };

} // namespace FoundationLocalisation
