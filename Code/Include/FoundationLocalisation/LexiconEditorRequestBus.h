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

#include <AzCore/EBus/EBus.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace FoundationLocalisation
{
    ///<summary>
    /// Editor-only bus that exposes the in-memory key tree built from scanned
    /// .helex source files. Used by LexiconPropertyHandler and LexiconKeyPickerDialog
    /// to populate the Key Picker overlay without re-parsing files on each open.
    ///
    /// Implemented by FoundationLocalisationEditorSystemComponent (singleton).
    ///</summary>
    class LexiconEditorRequests
    {
    public:
        AZ_RTTI(LexiconEditorRequests, "{B4C5D6E7-F8A9-4B1C-8D2E-3F4A5B6C7D8E}");
        virtual ~LexiconEditorRequests() = default;

        ///<summary>
        /// Returns all dot-path keys collected from every .helex file found in the
        /// project source tree. The list is sorted and deduplicated.
        /// Empty until the first scan completes (triggered on component Activate).
        ///</summary>
        virtual const AZStd::vector<AZStd::string>& GetKnownKeys() const = 0;

        ///<summary>
        /// Returns the absolute paths of every .helex source file found during the
        /// most recent scan. Used by LexiconToolWindow to populate the workspace
        /// dropdowns and perform super-set sync without re-scanning.
        ///</summary>
        virtual const AZStd::vector<AZStd::string>& GetKnownFilePaths() const = 0;

        ///<summary>
        /// Returns the project source root path used by the most recent scan.
        /// LexiconToolWindow uses this to write new .helex files to the correct
        /// location so they are found by the next scan.
        ///</summary>
        virtual AZStd::string GetProjectSourcePath() const = 0;

        ///<summary>
        /// Forces an immediate rescan of the project source tree for .helex files.
        /// Called automatically when a LexiconAssemblyAsset product is added or
        /// updated in the Asset Catalog (i.e. when .helex source files change).
        ///</summary>
        virtual void RefreshKeyTree() = 0;
    };

    class LexiconEditorRequestBusTraits : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    using LexiconEditorRequestBus = AZ::EBus<LexiconEditorRequests, LexiconEditorRequestBusTraits>;

} // namespace FoundationLocalisation
