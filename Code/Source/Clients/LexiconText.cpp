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

#include <FoundationLocalisation/LexiconText.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <xxHash/xxHashFunctions.h>

namespace Heathen
{
    void LexiconText::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<LexiconText>()
                ->Version(0)
                ->Field("Mode",       &LexiconText::m_mode)
                ->Field("KeyOrValue", &LexiconText::m_keyOrValue);

            if (auto* edit = serialize->GetEditContext())
            {
                edit->Class<LexiconText>("Lexicon Text", "Localisation-aware string field")
                    ->DataElement(nullptr, &LexiconText::m_mode,       "Mode",  "Localised, Literal or Invariant")
                    ->DataElement(nullptr, &LexiconText::m_keyOrValue, "Value", "Dot-path key (Localised) or raw string (Literal/Invariant)");
            }
        }

        if (auto* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behavior->Class<LexiconText>("Lexicon Text")
                ->Attribute(AZ::Script::Attributes::Category, "Localisation")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Constructor<>()
                ->Property("mode",        BehaviorValueProperty(&LexiconText::m_mode))
                ->Property("keyOrValue",  BehaviorValueProperty(&LexiconText::m_keyOrValue))
                ->Method("GetHash",       &LexiconText::GetHash)
                ->Method("InvalidateHash",&LexiconText::InvalidateHash)
                ->Method("IsLocalised",   &LexiconText::IsLocalised)
                ->Method("IsLiteral",     &LexiconText::IsLiteral)
                ->Method("IsInvariant",   &LexiconText::IsInvariant);
        }
    }

    AZ::u64 LexiconText::GetHash() const
    {
        if (m_cachedHash == 0 && !m_keyOrValue.empty())
        {
            m_cachedHash = xxHash::xxHashFunctions::Hash64(m_keyOrValue, 0);
        }
        return m_cachedHash;
    }

    void LexiconText::InvalidateHash()
    {
        m_cachedHash = 0;
    }

} // namespace Heathen
