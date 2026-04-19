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

#ifndef Q_MOC_RUN
#  include <AzCore/std/containers/vector.h>
#  include <AzCore/std/string/string.h>
#endif

namespace FoundationLocalisation
{
    ///<summary>
    /// Represents one Literal-mode Lexicon field found in a prefab.
    ///</summary>
    struct GatheredLiteral
    {
        AZStd::string m_filePath;       ///< Absolute path to the .prefab file
        AZStd::string m_currentValue;   ///< The raw string in m_keyOrValue
        AZStd::string m_proposedKey;    ///< Auto-suggested dot-path key; user may edit
    };

    ///<summary>
    /// Pure logic for the Gatherer tool — no Qt dependency.
    /// Scans project prefabs for Literal-mode Lexicon fields,
    /// writes confirmed entries to a .helex file, and patches
    /// the prefab JSON to flip those fields to Localised mode.
    ///</summary>
    class LexiconGathererWorker
    {
    public:
        ///<summary>
        /// Walks all .prefab files under the project source root.
        /// Returns one GatheredLiteral per unique (file, value) pair found with
        /// m_mode == LexiconLocMode::Literal (1).
        ///</summary>
        static AZStd::vector<GatheredLiteral> ScanPrefabs();

        ///<summary>
        /// Merges confirmed entries into the given .helex file.
        /// Creates the file with an empty cultures array if it does not exist.
        /// Never overwrites an existing entry key.
        /// Returns false if the file cannot be written.
        ///</summary>
        static bool WriteToHelex(
            const AZStd::string& helexPath,
            const AZStd::vector<GatheredLiteral>& entries);

        ///<summary>
        /// For each confirmed entry, rewrites every matching Literal-mode
        /// occurrence in its source prefab: m_mode → 0 (Localised),
        /// m_keyOrValue → proposedKey.
        ///</summary>
        static void PatchPrefabs(const AZStd::vector<GatheredLiteral>& entries);
    };

} // namespace FoundationLocalisation
