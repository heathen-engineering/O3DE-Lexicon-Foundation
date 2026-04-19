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

#include "LexiconValidator.h"

#include <QMap>

namespace FoundationLocalisation
{
    ValidationSummary LexiconValidator::Validate(LexiconEntryMap& map,
                                                  bool              hasSource,
                                                  bool              hasActive)
    {
        ValidationSummary summary;
        summary.total = map.size();

        // ---- Pass 1 — clear all flags, then set missing / orphan / isEmpty ----

        for (auto it = map.begin(); it != map.end(); ++it)
        {
            LexiconEntry& e = it.value();

            // Reset — guarantee a clean slate regardless of prior state
            e.isMissing   = false;
            e.isOrphan    = false;
            e.isDuplicate = false;
            e.isEmpty     = false;

            if (hasSource && hasActive)
            {
                // Missing: source has content but active does not
                e.isMissing = !e.sourceValue.isEmpty() && e.activeValue.isEmpty();

                // Orphan: active has content but source does not
                e.isOrphan  = e.sourceValue.isEmpty() && !e.activeValue.isEmpty();
            }

            // Empty: active file open, non-asset entry, active value is blank,
            //        and the key is not already flagged as missing or orphan
            if (hasActive && !e.isAsset && !e.isMissing && !e.isOrphan)
                e.isEmpty = e.activeValue.isEmpty();

            if (e.isMissing) ++summary.missing;
            if (e.isOrphan)  ++summary.orphans;
            if (e.isEmpty)   ++summary.empty;
        }

        // ---- Pass 2 — duplicate detection (same active value, different keys) ----
        //
        // Only runs when an active file is present. Asset UUID entries are excluded
        // — sharing a UUID across multiple keys is normal (e.g. same audio clip used
        // for two slightly different keys).

        if (hasActive)
        {
            // Count how many keys map to each non-empty string value
            QMap<QString, int> valueCounts;
            for (auto it = map.cbegin(); it != map.cend(); ++it)
            {
                const LexiconEntry& e = it.value();
                if (!e.isAsset && !e.activeValue.isEmpty())
                    valueCounts[e.activeValue]++;
            }

            // Flag any entry whose value is shared with at least one other key
            for (auto it = map.begin(); it != map.end(); ++it)
            {
                LexiconEntry& e = it.value();
                if (!e.isAsset && !e.activeValue.isEmpty()
                    && valueCounts.value(e.activeValue) > 1)
                {
                    e.isDuplicate = true;
                    ++summary.duplicates;
                }
            }
        }

        return summary;
    }

} // namespace FoundationLocalisation
