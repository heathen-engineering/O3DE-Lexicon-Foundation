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

#include <FoundationLocalisation/LexiconSound.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <xxHash/xxHashFunctions.h>

namespace Heathen
{
    void LexiconSound::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LexiconSound>()
                ->Version(0)
                ->Field("Mode",           &LexiconSound::m_mode)
                ->Field("KeyOrValue",     &LexiconSound::m_keyOrValue)
                ->Field("LiteralAssetId", &LexiconSound::m_literalAssetId);

            if (auto* edit = serialize->GetEditContext())
            {
                edit->Class<LexiconSound>("Lexicon Sound", "Localisation-aware sound asset reference")
                    ->DataElement(nullptr, &LexiconSound::m_mode,           "Mode",     "Localised, Literal or Invariant")
                    ->DataElement(nullptr, &LexiconSound::m_keyOrValue,     "Key",      "Dot-path key (Localised mode only)")
                    ->DataElement(nullptr, &LexiconSound::m_literalAssetId, "Asset ID", "Sound asset UUID (Literal/Invariant mode)");
            }
        }

        if (auto* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behavior->Class<LexiconSound>("Lexicon Sound")
                ->Attribute(AZ::Script::Attributes::Category, "Localisation")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Constructor<>()
                ->Property("mode",           BehaviorValueProperty(&LexiconSound::m_mode))
                ->Property("keyOrValue",     BehaviorValueProperty(&LexiconSound::m_keyOrValue))
                ->Property("literalAssetId", BehaviorValueProperty(&LexiconSound::m_literalAssetId))
                ->Method("GetHash",          &LexiconSound::GetHash)
                ->Method("InvalidateHash",   &LexiconSound::InvalidateHash)
                ->Method("IsLocalised",      &LexiconSound::IsLocalised)
                ->Method("IsLiteral",        &LexiconSound::IsLiteral)
                ->Method("IsInvariant",      &LexiconSound::IsInvariant);
        }
    }

    AZ::u64 LexiconSound::GetHash() const
    {
        if (m_cachedHash == 0 && !m_keyOrValue.empty())
        {
            m_cachedHash = xxHash::xxHashFunctions::Hash64(m_keyOrValue, 0);
        }
        return m_cachedHash;
    }

    void LexiconSound::InvalidateHash()
    {
        m_cachedHash = 0;
    }

} // namespace Heathen
