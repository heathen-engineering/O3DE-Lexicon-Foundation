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

#include <AzCore/base.h>

namespace FoundationLocalisation
{
    ///<summary>
    /// Editor-only hint that describes the intended content type of a .helex entry.
    /// Stored in the .helex source JSON ("hint" field) and cached by
    /// FoundationLocalisationEditorSystemComponent at scan time.
    /// Never written to the compiled .lexicon binary.
    ///
    /// Used by LexiconKeyPickerDialog to filter the key list to only the entry
    /// types relevant to the property being edited:
    ///   String — plain text (LexiconText)
    ///   Sound  — sound asset UUID (LexiconSound)
    ///   Asset  — generic asset UUID (LexiconAsset)
    ///
    /// None is returned for keys where no hint is derivable (should not normally
    /// occur with well-formed .helex files). The picker shows all keys when None
    /// is used as a filter.
    ///</summary>
    enum class LexiconHintType : AZ::u8
    {
        None      = 0, // no hint — bare string entry with no explicit hint field
        String    = 1, // plain UTF-8 text
        Sound     = 2, // sound asset UUID
        Texture   = 3, // texture asset UUID
        Spawnable = 4, // AzFramework::Spawnable asset UUID
        Asset     = 5, // generic asset UUID (catch-all)
    };

} // namespace FoundationLocalisation
