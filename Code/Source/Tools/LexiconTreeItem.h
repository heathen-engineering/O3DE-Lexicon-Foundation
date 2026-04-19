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

#include <memory>
#include <vector>

namespace FoundationLocalisation
{
    ///<summary>
    /// One node in the LexiconTreeModel hierarchy.
    ///
    /// Intermediate node  — represents a dot-path segment (e.g. "Menu" in "Menu.Play").
    ///                      Has children; IsLeaf() returns false.
    ///
    /// Leaf node           — represents a key that actually exists in at least one of
    ///                       the open .helex files. IsLeaf() returns true; sourceValue /
    ///                       activeValue / isAsset carry the parsed content.
    ///
    /// Note: a segment can be BOTH an intermediate node AND a leaf — e.g. if "Menu"
    /// and "Menu.Play" both exist as keys. In that case IsLeaf() returns true and
    /// the node also has children.
    ///</summary>
    class LexiconTreeItem
    {
    public:
        explicit LexiconTreeItem(const QString& segment,
                                  const QString& fullPath,
                                  LexiconTreeItem* parent = nullptr);

        // ---- Tree structure --------------------------------------------------

        void             AddChild(std::unique_ptr<LexiconTreeItem> child);

        ///<summary>Finds a direct child by its segment name. Returns nullptr if not found.</summary>
        LexiconTreeItem* FindChild(const QString& segment) const;

        LexiconTreeItem* Parent()     const { return m_parent; }
        LexiconTreeItem* Child(int row) const;
        int              ChildCount() const { return static_cast<int>(m_children.size()); }

        ///<summary>Returns this node's row index within its parent's children list.</summary>
        int              Row() const;

        // ---- Identity -------------------------------------------------------

        const QString& Segment()  const { return m_segment; }
        const QString& FullPath() const { return m_fullPath; }
        bool           IsLeaf()   const { return m_isLeaf; }
        void           MarkAsLeaf()     { m_isLeaf = true; }

        // ---- Leaf data (only meaningful when IsLeaf() == true) --------------

        QString sourceValue;
        QString activeValue;
        bool    isAsset     = false;

        // ---- Validation badge (set by LexiconTreeModel::ApplyValidationFlags) ----

        ///<summary>
        /// For leaf nodes: true when any validation flag (isMissing, isOrphan,
        /// isDuplicate, isEmpty) is set on the corresponding LexiconEntry.
        /// For intermediate nodes: true when any descendant leaf has an issue.
        /// Drives ForegroundRole colouring in LexiconTreeModel::data().
        ///</summary>
        bool    hasIssues   = false;

    private:
        QString          m_segment;
        QString          m_fullPath;
        LexiconTreeItem* m_parent  = nullptr;
        bool             m_isLeaf  = false;

        std::vector<std::unique_ptr<LexiconTreeItem>> m_children;
    };

} // namespace FoundationLocalisation
