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

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/string.h>

#include <FoundationLocalisation/FoundationLocalisationTypeIds.h>
#include <FoundationLocalisation/LexiconLocMode.h>

namespace Heathen
{
    ///<summary>
    /// A localisation-aware string field for use in components and assets.
    ///
    /// Localised  — m_keyOrValue holds a dot-path key (e.g. "Menu.Play").
    ///              Call GetHash() to obtain the u64 key for bus lookups.
    ///              Read-only in the Inspector; use the Key Picker to change.
    ///
    /// Literal    — m_keyOrValue holds a raw string entered directly.
    ///              Behaves like a standard string field.
    ///
    /// Invariant  — m_keyOrValue holds a raw string. Explicitly excluded from
    ///              the Gatherer tool. Use for identifiers that must not be
    ///              localised.
    ///</summary>
    class LexiconText
    {
    public:
        AZ_TYPE_INFO(LexiconText, LexiconTextTypeId)
        AZ_CLASS_ALLOCATOR(LexiconText, AZ::SystemAllocator, 0)

        LexiconText() = default;

        static void Reflect(AZ::ReflectContext* context);

        ///<summary>
        /// Returns the XXH3_64bits hash of m_keyOrValue, computing and caching it on
        /// first call. Only meaningful when m_mode == Localised.
        ///</summary>
        AZ::u64 GetHash() const;

        ///<summary>
        /// Clears the cached hash. Call after modifying m_keyOrValue directly (e.g.
        /// via editor serialisation).
        ///</summary>
        void InvalidateHash();

        bool IsLocalised() const { return m_mode == LexiconLocMode::Localised; }
        bool IsLiteral()   const { return m_mode == LexiconLocMode::Literal; }
        bool IsInvariant() const { return m_mode == LexiconLocMode::Invariant; }

        LexiconLocMode m_mode       = LexiconLocMode::Literal;
        AZStd::string  m_keyOrValue;

    private:
        mutable AZ::u64 m_cachedHash = 0;
    };

} // namespace Heathen
