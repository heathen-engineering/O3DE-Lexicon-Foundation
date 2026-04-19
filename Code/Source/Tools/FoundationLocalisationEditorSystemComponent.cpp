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

#include "FoundationLocalisationEditorSystemComponent.h"
#include "LexiconToolWindow.h"
#include "LexiconPropertyHandler.h"

#include <FoundationLocalisation/FoundationLocalisationTypeIds.h>
#include <FoundationLocalisation/LexiconAssemblyAsset.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <algorithm>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
AZ_POP_DISABLE_WARNING

namespace FoundationLocalisation
{
    AZ_COMPONENT_IMPL(FoundationLocalisationEditorSystemComponent, "FoundationLocalisationEditorSystemComponent",
        FoundationLocalisationEditorSystemComponentTypeId, BaseSystemComponent);

    void FoundationLocalisationEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<FoundationLocalisationEditorSystemComponent, FoundationLocalisationSystemComponent>()
                ->Version(0);
        }
    }

    FoundationLocalisationEditorSystemComponent::FoundationLocalisationEditorSystemComponent() = default;

    FoundationLocalisationEditorSystemComponent::~FoundationLocalisationEditorSystemComponent() = default;

    void FoundationLocalisationEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("FoundationLocalisationEditorService"));
    }

    void FoundationLocalisationEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("FoundationLocalisationEditorService"));
    }

    void FoundationLocalisationEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void FoundationLocalisationEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void FoundationLocalisationEditorSystemComponent::Activate()
    {
        FoundationLocalisationSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
        LexiconEditorRequestBus::Handler::BusConnect();
        AzFramework::AssetCatalogEventBus::Handler::BusConnect();

        // Register asset type handler so the AssetManager can load .lexicon files
        m_assetHandler = aznew AzFramework::GenericAssetHandler<Heathen::LexiconAssemblyAsset>(
            "Lexicon Assembly", "Lexicon Localisation", "lexicon");
        m_assetHandler->Register();

        // Register the asset builder so the Asset Processor watches .helex files
        m_builder.RegisterBuilder();

        // Register property handlers for the three Lexicon data types
        m_textHandler  = aznew LexiconTextPropertyHandler();
        m_soundHandler = aznew LexiconSoundPropertyHandler();
        m_assetPropertyHandler = aznew LexiconAssetPropertyHandler();

        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType,
            m_textHandler);
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType,
            m_soundHandler);
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyTypeRegistrationMessages::RegisterPropertyType,
            m_assetPropertyHandler);

        // Build the initial key tree from whatever .helex files exist on disk now
        ScanSourceFiles();
    }

    void FoundationLocalisationEditorSystemComponent::Deactivate()
    {
        AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
        LexiconEditorRequestBus::Handler::BusDisconnect();

        // Unregister and destroy property handlers
        if (m_textHandler)
        {
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType,
                m_textHandler);
            delete m_textHandler;
            m_textHandler = nullptr;
        }
        if (m_soundHandler)
        {
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType,
                m_soundHandler);
            delete m_soundHandler;
            m_soundHandler = nullptr;
        }
        if (m_assetPropertyHandler)
        {
            AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
                &AzToolsFramework::PropertyTypeRegistrationMessages::UnregisterPropertyType,
                m_assetPropertyHandler);
            delete m_assetPropertyHandler;
            m_assetPropertyHandler = nullptr;
        }

        m_builder.BusDisconnect();

        if (m_assetHandler)
        {
            m_assetHandler->Unregister();
            delete m_assetHandler;
            m_assetHandler = nullptr;
        }

        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        FoundationLocalisationSystemComponent::Deactivate();
    }

    void FoundationLocalisationEditorSystemComponent::NotifyRegisterViews()
    {
        AzToolsFramework::ViewPaneOptions options;
        options.paneRect           = QRect(100, 100, 500, 400);
        options.showOnToolsToolbar = true;
        options.toolbarIcon        = ":/FoundationLocalisation/toolbar_icon.svg";
        options.showInMenu         = false; // menu handled by Action Manager hooks

        AzToolsFramework::RegisterViewPane<LexiconToolWindow>(
            "Lexicon", "Heathen Tools", options);
    }

    // ── Action Manager hooks ────────────────────────────────────────────────────

    void FoundationLocalisationEditorSystemComponent::OnMenuRegistrationHook()
    {
        auto* menuManager = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        if (!menuManager)
            return;

        if (!menuManager->IsMenuRegistered("heathen.menu.tools"))
        {
            AzToolsFramework::MenuProperties props;
            props.m_name = "Heathen Tools";
            menuManager->RegisterMenu("heathen.menu.tools", props);
        }
    }

    void FoundationLocalisationEditorSystemComponent::OnMenuBindingHook()
    {
        auto* menuManager = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        if (!menuManager)
            return;

        static bool s_submenuAdded = false;
        if (!s_submenuAdded)
        {
            menuManager->AddSubMenuToMenu(
                AZStd::string(EditorIdentifiers::ToolsMenuIdentifier),
                "heathen.menu.tools", 9000);
            s_submenuAdded = true;
        }

        menuManager->AddActionToMenu("heathen.menu.tools",
            "heathen.action.lexicon", 300);
    }

    void FoundationLocalisationEditorSystemComponent::OnActionRegistrationHook()
    {
        auto* actionManager = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        if (!actionManager)
            return;

        AzToolsFramework::ActionProperties props;
        props.m_name        = "Lexicon";
        props.m_description = "Open the Lexicon localisation editor";
        props.m_category    = "Heathen Tools";

        actionManager->RegisterAction(
            AZStd::string(EditorIdentifiers::MainWindowActionContextIdentifier),
            "heathen.action.lexicon",
            props,
            []()
            {
                AzToolsFramework::OpenViewPane("Lexicon");
            });
    }

    ////////////////////////////////////////////////////////////////////////
    // LexiconEditorRequestBus

    const AZStd::vector<AZStd::string>& FoundationLocalisationEditorSystemComponent::GetKnownKeys() const
    {
        return m_knownKeys;
    }

    const AZStd::vector<AZStd::string>& FoundationLocalisationEditorSystemComponent::GetKnownFilePaths() const
    {
        return m_knownFilePaths;
    }

    AZStd::string FoundationLocalisationEditorSystemComponent::GetProjectSourcePath() const
    {
        return m_projectSourcePath;
    }

    void FoundationLocalisationEditorSystemComponent::RefreshKeyTree()
    {
        ScanSourceFiles();
    }

    ////////////////////////////////////////////////////////////////////////
    // AzFramework::AssetCatalogEventBus

    void FoundationLocalisationEditorSystemComponent::OnCatalogAssetAdded(const AZ::Data::AssetId& assetId)
    {
        CheckAndRefresh(assetId);
    }

    void FoundationLocalisationEditorSystemComponent::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
    {
        CheckAndRefresh(assetId);
    }

    ////////////////////////////////////////////////////////////////////////
    // Private helpers

    void FoundationLocalisationEditorSystemComponent::CheckAndRefresh(const AZ::Data::AssetId& assetId)
    {
        AZ::Data::AssetInfo info;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            info, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);

        if (info.m_assetType == azrtti_typeid<Heathen::LexiconAssemblyAsset>())
        {
            ScanSourceFiles();
        }
    }

    void FoundationLocalisationEditorSystemComponent::ScanSourceFiles()
    {
        m_knownKeys.clear();
        m_knownFilePaths.clear();

        // Resolve the project source root
        AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
        if (projectPath.empty())
        {
            AZ_Warning("FoundationLocalisation", false,
                "LexiconEditorRequestBus: project path is empty — key tree will be empty.");
            return;
        }

        m_projectSourcePath = AZStd::string(projectPath.c_str());

        // Recursively collect all .helex files under the project root.
        // Use ResolvePath() on every result so callers always receive native
        // filesystem paths, never @projectroot@ or other engine aliases.
        AZStd::function<void(const AZStd::string&)> collectRecursive =
            [&](const AZStd::string& dir)
            {
                auto* fileIO = AZ::IO::FileIOBase::GetInstance();
                if (!fileIO)
                    return;

                // Collect .helex files in this directory
                fileIO->FindFiles(dir.c_str(), "*.helex",
                    [&](const char* path) -> bool
                    {
                        AZ::IO::FixedMaxPath resolved;
                        if (fileIO->ResolvePath(resolved, path))
                            m_knownFilePaths.emplace_back(resolved.c_str());
                        else
                            m_knownFilePaths.emplace_back(path);
                        return true;
                    });

                // Recurse into subdirectories
                fileIO->FindFiles(dir.c_str(), "*",
                    [&collectRecursive](const char* path) -> bool
                    {
                        if (AZ::IO::FileIOBase::GetInstance()->IsDirectory(path))
                        {
                            collectRecursive(path);
                        }
                        return true;
                    });
            };

        collectRecursive(AZStd::string(projectPath.c_str()));

        // Parse keys from each file
        for (const AZStd::string& filePath : m_knownFilePaths)
        {
            ParseHelexKeys(filePath);
        }

        // Sort and deduplicate so the picker shows a clean ordered list
        std::sort(m_knownKeys.begin(), m_knownKeys.end());
        m_knownKeys.erase(
            AZStd::unique(m_knownKeys.begin(), m_knownKeys.end()),
            m_knownKeys.end());

        AZ_TracePrintf("FoundationLocalisation",
            "LexiconEditorRequestBus: %zu keys loaded from %zu .helex file(s).",
            m_knownKeys.size(), m_knownFilePaths.size());
    }

    void FoundationLocalisationEditorSystemComponent::ParseHelexKeys(const AZStd::string& filePath)
    {
        auto* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
            return;

        AZ::IO::HandleType handle = AZ::IO::InvalidHandle;
        if (fileIO->Open(filePath.c_str(),
                AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary,
                handle) != AZ::IO::ResultCode::Success)
        {
            AZ_Warning("FoundationLocalisation", false,
                "LexiconEditorRequestBus: could not open '%s'.", filePath.c_str());
            return;
        }

        AZ::u64 fileSize = 0;
        fileIO->Size(handle, fileSize);

        AZStd::vector<char> content(static_cast<size_t>(fileSize) + 1, '\0');
        fileIO->Read(handle, content.data(), fileSize);
        fileIO->Close(handle);

        rapidjson::Document doc;
        doc.ParseInsitu(content.data());

        if (doc.HasParseError() || !doc.IsObject())
        {
            AZ_Warning("FoundationLocalisation", false,
                "LexiconEditorRequestBus: '%s' is not valid JSON — skipping.",
                filePath.c_str());
            return;
        }

        if (!doc.HasMember("entries") || !doc["entries"].IsObject())
        {
            return;
        }

        for (auto it = doc["entries"].MemberBegin(); it != doc["entries"].MemberEnd(); ++it)
        {
            if (it->name.IsString())
            {
                m_knownKeys.emplace_back(it->name.GetString(), it->name.GetStringLength());
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////

} // namespace FoundationLocalisation
