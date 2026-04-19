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

#include <FoundationLocalisation/LexiconLocMode.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace Heathen
{
    void ReflectLexiconLocMode(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Enum<LexiconLocMode>()
                ->Version(0)
                ->Value("Localised", LexiconLocMode::Localised)
                ->Value("Literal",   LexiconLocMode::Literal)
                ->Value("Invariant", LexiconLocMode::Invariant);
        }

        if (auto* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behavior->Enum<static_cast<int>(LexiconLocMode::Localised)>("LexiconLocMode_Localised")
                    ->Enum<static_cast<int>(LexiconLocMode::Literal)>("LexiconLocMode_Literal")
                    ->Enum<static_cast<int>(LexiconLocMode::Invariant)>("LexiconLocMode_Invariant");
        }
    }

} // namespace Heathen
