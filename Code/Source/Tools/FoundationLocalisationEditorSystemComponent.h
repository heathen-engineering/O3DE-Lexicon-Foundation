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

#include <Clients/FoundationLocalisationSystemComponent.h>
#include "LexiconAssemblyAssetBuilder.h"
#include "LexiconEditorRequestBus.h"
#include "LexiconPropertyHandler.h"

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Asset/GenericAssetHandler.h>
#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

#include <FoundationLocalisation/LexiconAssemblyAsset.h>

namespace FoundationLocalisation
{
    class FoundationLocalisationEditorSystemComponent
        : public FoundationLocalisationSystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
        , public AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
        , public LexiconEditorRequestBus::Handler
        , public AzFramework::AssetCatalogEventBus::Handler
    {
        using BaseSystemComponent = FoundationLocalisationSystemComponent;
    public:
        AZ_COMPONENT_DECL(FoundationLocalisationEditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        FoundationLocalisationEditorSystemComponent();
        ~FoundationLocalisationEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AzToolsFramework::EditorEventsBus
        void NotifyRegisterViews() override;

        // AzToolsFramework::ActionManagerRegistrationNotificationBus
        void OnMenuRegistrationHook()   override;
        void OnMenuBindingHook()        override;
        void OnActionRegistrationHook() override;

        ////////////////////////////////////////////////////////////////////////
        // LexiconEditorRequestBus
        const AZStd::vector<AZStd::string>& GetKnownKeys()      const override;
        const AZStd::vector<AZStd::string>& GetKnownFilePaths() const override;
        AZStd::string                        GetProjectSourcePath() const override;
        void RefreshKeyTree() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AzFramework::AssetCatalogEventBus
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;
        ////////////////////////////////////////////////////////////////////////

        ///<summary>
        /// Walks the project source tree for *.helex files, parses their entries
        /// keys, and rebuilds m_knownKeys (sorted, deduplicated).
        ///</summary>
        void ScanSourceFiles();

        ///<summary>
        /// Parses a single .helex file and appends its entry keys to m_knownKeys.
        ///</summary>
        void ParseHelexKeys(const AZStd::string& filePath);

        ///<summary>
        /// Checks whether the given asset ID is a LexiconAssemblyAsset product;
        /// if so, triggers a source file rescan.
        ///</summary>
        void CheckAndRefresh(const AZ::Data::AssetId& assetId);

        LexiconAssemblyAssetBuilder m_builder;
        AzFramework::GenericAssetHandler<Heathen::LexiconAssemblyAsset>* m_assetHandler = nullptr;

        LexiconTextPropertyHandler*  m_textHandler  = nullptr;
        LexiconSoundPropertyHandler* m_soundHandler = nullptr;
        LexiconAssetPropertyHandler* m_assetPropertyHandler = nullptr;

        AZStd::vector<AZStd::string> m_knownKeys;
        AZStd::vector<AZStd::string> m_knownFilePaths;
        AZStd::string                m_projectSourcePath;
    };

} // namespace FoundationLocalisation
