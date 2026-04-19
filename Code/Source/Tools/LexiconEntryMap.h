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

#include <QMap>
#include <QString>

namespace FoundationLocalisation
{
    ///<summary>
    /// Represents one key entry built from the two open .helex files.
    /// Shared between LexiconTreeModel (Explorer) and LexiconTableModel (Workbench).
    ///</summary>
    struct LexiconEntry
    {
        QString sourceValue;        ///< Value from the source (reference) .helex; empty if key absent
        QString activeValue;        ///< Value from the active (editable) .helex; empty if key absent
        bool    isAsset  = false;   ///< True when the value is a UUID asset reference, not a string

        // Validation flags — populated by LexiconValidator (Phase 6.6)
        bool    isMissing   = false; ///< Key present in source but absent in active
        bool    isOrphan    = false; ///< Key present in active but absent in source
        bool    isDuplicate = false; ///< Same value used under multiple different keys
        bool    isEmpty     = false; ///< Key present but value is an empty string
    };

    ///<summary>
    /// Sorted map of dot-path key → LexiconEntry.
    /// QMap preserves alphabetical order, which the tree-building algorithm relies on
    /// to process parent segments before their children.
    ///</summary>
    using LexiconEntryMap = QMap<QString, LexiconEntry>;

} // namespace FoundationLocalisation
