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

#include <FoundationLocalisation/LexiconAsset.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <xxHash/xxHashFunctions.h>

namespace Heathen
{
    void LexiconAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LexiconAsset>()
                ->Version(0)
                ->Field("Mode",           &LexiconAsset::m_mode)
                ->Field("KeyOrValue",     &LexiconAsset::m_keyOrValue)
                ->Field("LiteralAssetId", &LexiconAsset::m_literalAssetId);

            if (auto* edit = serialize->GetEditContext())
            {
                edit->Class<LexiconAsset>("Lexicon Asset", "Localisation-aware generic asset reference")
                    ->DataElement(nullptr, &LexiconAsset::m_mode,           "Mode",     "Localised, Literal or Invariant")
                    ->DataElement(nullptr, &LexiconAsset::m_keyOrValue,     "Key",      "Dot-path key (Localised mode only)")
                    ->DataElement(nullptr, &LexiconAsset::m_literalAssetId, "Asset ID", "Asset UUID (Literal/Invariant mode)");
            }
        }

        if (auto* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behavior->Class<LexiconAsset>("Lexicon Asset")
                ->Attribute(AZ::Script::Attributes::Category, "Localisation")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Constructor<>()
                ->Property("mode",           BehaviorValueProperty(&LexiconAsset::m_mode))
                ->Property("keyOrValue",     BehaviorValueProperty(&LexiconAsset::m_keyOrValue))
                ->Property("literalAssetId", BehaviorValueProperty(&LexiconAsset::m_literalAssetId))
                ->Method("GetHash",          &LexiconAsset::GetHash)
                ->Method("InvalidateHash",   &LexiconAsset::InvalidateHash)
                ->Method("IsLocalised",      &LexiconAsset::IsLocalised)
                ->Method("IsLiteral",        &LexiconAsset::IsLiteral)
                ->Method("IsInvariant",      &LexiconAsset::IsInvariant);
        }
    }

    AZ::u64 LexiconAsset::GetHash() const
    {
        if (m_cachedHash == 0 && !m_keyOrValue.empty())
        {
            m_cachedHash = xxHash::xxHashFunctions::Hash64(m_keyOrValue, 0);
        }
        return m_cachedHash;
    }

    void LexiconAsset::InvalidateHash()
    {
        m_cachedHash = 0;
    }

} // namespace Heathen
