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

#include <AzCore/Serialization/SerializeContext.h>

namespace Heathen
{
    ///<summary>
    /// Controls how a Lexicon localisation field resolves its value at runtime.
    ///
    /// Localised — the field holds a dot-path key (e.g. "Menu.Play"). The runtime
    ///             resolves it against the active LexiconAssemblyAsset. Read-only
    ///             in the Editor Inspector; change the value via the Key Picker.
    ///
    /// Literal    — the field holds a raw value entered directly. No lookup is
    ///             performed. Behaves like a standard (non-localised) field.
    ///
    /// Invariant  — the field holds a raw value and is explicitly flagged to be
    ///             ignored by the Gatherer tool. Use this for values that must
    ///             never be localised (e.g. internal identifiers, debug strings).
    ///</summary>
    enum class LexiconLocMode : AZ::u8
    {
        Localised = 0,
        Literal   = 1,
        Invariant = 2,
    };

    /// Registers LexiconLocMode with SerializeContext and BehaviorContext.
    /// Call this once from FoundationLocalisationSystemComponent::Reflect().
    void ReflectLexiconLocMode(AZ::ReflectContext* context);

} // namespace Heathen

// Q_MOC_RUN guard: AZ_TYPE_INFO_SPECIALIZE expands using macros defined inside
// namespace AZ, which confuses Qt MOC's simple parser into thinking the following
// namespace declarations are nested inside AZ.
#ifndef Q_MOC_RUN
AZ_TYPE_INFO_SPECIALIZE(Heathen::LexiconLocMode, "{E7F8091A-2B3C-4D5E-6F70-8192A3B4C5D6}");
#endif
