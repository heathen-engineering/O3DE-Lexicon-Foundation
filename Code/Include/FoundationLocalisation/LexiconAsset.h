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

#include <AzCore/Math/Uuid.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/string.h>

#include <FoundationLocalisation/FoundationLocalisationTypeIds.h>
#include <FoundationLocalisation/LexiconLocMode.h>

namespace Heathen
{
    ///<summary>
    /// A localisation-aware generic asset reference for use in components and assets.
    ///
    /// Localised  — m_keyOrValue holds a dot-path key (e.g. "UI.Logo").
    ///              The key resolves to an AZ::Uuid (asset ID) via the active
    ///              LexiconAssemblyAsset. Call GetHash() for bus lookups.
    ///
    /// Literal    — m_literalAssetId holds the asset UUID directly.
    ///              m_keyOrValue is unused.
    ///
    /// Invariant  — Same as Literal but excluded from the Gatherer tool.
    ///</summary>
    class LexiconAsset
    {
    public:
        AZ_TYPE_INFO(LexiconAsset, LexiconAssetTypeId)
        AZ_CLASS_ALLOCATOR(LexiconAsset, AZ::SystemAllocator, 0)

        LexiconAsset() = default;

        static void Reflect(AZ::ReflectContext* context);

        ///<summary>
        /// Returns the XXH3_64bits hash of m_keyOrValue, computing and caching it on
        /// first call. Only meaningful when m_mode == Localised.
        ///</summary>
        AZ::u64 GetHash() const;

        ///<summary>
        /// Clears the cached hash. Call after modifying m_keyOrValue directly.
        ///</summary>
        void InvalidateHash();

        bool IsLocalised() const { return m_mode == LexiconLocMode::Localised; }
        bool IsLiteral()   const { return m_mode == LexiconLocMode::Literal; }
        bool IsInvariant() const { return m_mode == LexiconLocMode::Invariant; }

        LexiconLocMode m_mode           = LexiconLocMode::Literal;
        AZStd::string  m_keyOrValue;
        AZ::Uuid       m_literalAssetId = AZ::Uuid::CreateNull();

    private:
        mutable AZ::u64 m_cachedHash = 0;
    };

} // namespace Heathen
