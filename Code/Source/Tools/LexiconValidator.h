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

namespace FoundationLocalisation
{
    ///<summary>
    /// Counts returned by LexiconValidator::Validate() for display in the status bar.
    ///</summary>
    struct ValidationSummary
    {
        int total      = 0; ///< Total number of keys in the map
        int missing    = 0; ///< Keys present in source but empty in active
        int orphans    = 0; ///< Keys present in active but absent in source
        int duplicates = 0; ///< Keys whose active value is shared with another key
        int empty      = 0; ///< Keys with an empty active value (not accounted for by missing/orphan)

        bool HasIssues() const { return missing > 0 || orphans > 0 || duplicates > 0 || empty > 0; }
    };

    ///<summary>
    /// Stateless validator for a LexiconEntryMap.
    ///
    /// Validate() mutates the map in place, setting the four boolean flags on each
    /// LexiconEntry and returning a ValidationSummary with aggregate counts.
    ///
    /// Rules:
    ///   isMissing   — sourceValue is non-empty AND activeValue is empty.
    ///                 Only set when both a source and active file are open.
    ///   isOrphan    — activeValue is non-empty AND sourceValue is empty.
    ///                 Only set when both a source and active file are open.
    ///   isDuplicate — activeValue is shared (case-sensitive) by two or more
    ///                 different keys. Only set for string entries (not assets)
    ///                 and only when an active file is open.
    ///   isEmpty     — activeValue is an empty string for a string entry (not asset),
    ///                 and neither isMissing nor isOrphan apply.
    ///                 Only set when an active file is open.
    ///
    /// Flags are cleared (reset to false) at the start of each call, so the map
    /// returned by a previous parse pass is left in a clean state.
    ///</summary>
    class LexiconValidator
    {
    public:
        ///<summary>
        /// Validates every entry in map, sets the four bool flags in place, and
        /// returns aggregate counts.
        ///
        /// hasSource — a reference .helex file is currently open in the tool.
        /// hasActive — an editable .helex file is currently open in the tool.
        ///</summary>
        static ValidationSummary Validate(LexiconEntryMap& map, bool hasSource, bool hasActive);
    };

} // namespace FoundationLocalisation
