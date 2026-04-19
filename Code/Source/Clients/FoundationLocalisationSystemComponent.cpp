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

#include "FoundationLocalisationSystemComponent.h"

#include <FoundationLocalisation/FoundationLocalisationTypeIds.h>
#include <FoundationLocalisation/LexiconLocMode.h>
#include <FoundationLocalisation/LexiconAssemblyAsset.h>
#include <FoundationLocalisation/LexiconText.h>
#include <FoundationLocalisation/LexiconSound.h>
#include <FoundationLocalisation/LexiconAsset.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

#include <xxHash/xxHashFunctions.h>

namespace FoundationLocalisation
{
    AZ_COMPONENT_IMPL(FoundationLocalisationSystemComponent, "FoundationLocalisationSystemComponent",
        FoundationLocalisationSystemComponentTypeId);

    void FoundationLocalisationSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        // Reflect all Heathen types owned by this gem
        Heathen::ReflectLexiconLocMode(context);
        Heathen::LexiconAssemblyAsset::Reflect(context);
        Heathen::LexiconText::Reflect(context);
        Heathen::LexiconSound::Reflect(context);
        Heathen::LexiconAsset::Reflect(context);

        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<FoundationLocalisationSystemComponent, AZ::Component>()
                ->Version(0)
                ->Field("ActiveCulture",  &FoundationLocalisationSystemComponent::m_activeCulture)
                ->Field("DefaultCulture", &FoundationLocalisationSystemComponent::m_defaultCulture);
        }

        if (auto* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // Expose overloaded methods with distinct SC node names via explicit casts.
            behavior->EBus<FoundationLocalisationRequestBus>("Lexicon Localisation")
                ->Attribute(AZ::Script::Attributes::Category, "Localisation")
                ->Event("Resolve String",
                    static_cast<AZStd::string(FoundationLocalisationRequests::*)(AZ::u64)>(
                        &FoundationLocalisationRequests::ResolveString),
                    {{{ "Key", "Pre-hashed u64 key (use Get Hash on a Lexicon type)" }}})
                ->Event("Resolve String By Path",
                    static_cast<AZStd::string(FoundationLocalisationRequests::*)(const AZStd::string&)>(
                        &FoundationLocalisationRequests::ResolveString),
                    {{{ "Path", "Dot-path key string e.g. Menu.Play" }}})
                ->Event("Resolve Asset Id",
                    static_cast<AZ::Uuid(FoundationLocalisationRequests::*)(AZ::u64)>(
                        &FoundationLocalisationRequests::ResolveAssetId),
                    {{{ "Key", "Pre-hashed u64 key" }}})
                ->Event("Resolve Asset Id By Path",
                    static_cast<AZ::Uuid(FoundationLocalisationRequests::*)(const AZStd::string&)>(
                        &FoundationLocalisationRequests::ResolveAssetId),
                    {{{ "Path", "Dot-path key string e.g. UI.Logo" }}})
                ->Event("Load Culture",
                    &FoundationLocalisationRequests::LoadCulture,
                    {{{ "Culture Code", "IETF culture code e.g. en-GB" }}})
                ->Event("Get Active Culture",
                    &FoundationLocalisationRequests::GetActiveCulture)
                ->Event("Set Default Culture",
                    &FoundationLocalisationRequests::SetDefaultCulture,
                    {{{ "Culture Code", "IETF culture code e.g. en-GB" }}})
                ->Event("Get Available Lexicon Ids",
                    &FoundationLocalisationRequests::GetAvailableLexiconIds)
                ->Event("Get Lexicon Display Name",
                    &FoundationLocalisationRequests::GetLexiconDisplayName,
                    {{{ "Asset Id", "The assetId of the lexicon e.g. French_Standard" }}});
        }
    }

    void FoundationLocalisationSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("FoundationLocalisationService"));
    }

    void FoundationLocalisationSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("FoundationLocalisationService"));
    }

    void FoundationLocalisationSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void FoundationLocalisationSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    FoundationLocalisationSystemComponent::FoundationLocalisationSystemComponent()
    {
        if (FoundationLocalisationInterface::Get() == nullptr)
        {
            FoundationLocalisationInterface::Register(this);
        }
    }

    FoundationLocalisationSystemComponent::~FoundationLocalisationSystemComponent()
    {
        if (FoundationLocalisationInterface::Get() == this)
        {
            FoundationLocalisationInterface::Unregister(this);
        }
    }

    void FoundationLocalisationSystemComponent::Init()
    {
    }

    void FoundationLocalisationSystemComponent::Activate()
    {
        FoundationLocalisationRequestBus::Handler::BusConnect();
    }

    void FoundationLocalisationSystemComponent::Deactivate()
    {
        AZ::Data::AssetBus::MultiHandler::BusDisconnect();
        FoundationLocalisationRequestBus::Handler::BusDisconnect();

        m_activeAsset.Release();
        m_defaultAsset.Release();
        m_allLoadedAssets.clear();
    }

    ////////////////////////////////////////////////////////////////////////
    // FoundationLocalisationRequestBus

    AZStd::string FoundationLocalisationSystemComponent::ResolveString(AZ::u64 key)
    {
        if (m_activeAsset.IsReady())
        {
            AZStd::string result = m_activeAsset->FindString(key);
            if (!result.empty())
            {
                return result;
            }
        }

        if (m_defaultAsset.IsReady())
        {
            return m_defaultAsset->FindString(key);
        }

        return {};
    }

    AZStd::string FoundationLocalisationSystemComponent::ResolveString(const AZStd::string& path)
    {
        return ResolveString(xxHash::xxHashFunctions::Hash64(path, 0));
    }

    AZ::Uuid FoundationLocalisationSystemComponent::ResolveAssetId(AZ::u64 key)
    {
        if (m_activeAsset.IsReady())
        {
            AZ::Uuid result = m_activeAsset->FindAssetId(key);
            if (!result.IsNull())
            {
                return result;
            }
        }

        if (m_defaultAsset.IsReady())
        {
            return m_defaultAsset->FindAssetId(key);
        }

        return AZ::Uuid::CreateNull();
    }

    AZ::Uuid FoundationLocalisationSystemComponent::ResolveAssetId(const AZStd::string& path)
    {
        return ResolveAssetId(xxHash::xxHashFunctions::Hash64(path, 0));
    }

    void FoundationLocalisationSystemComponent::LoadCulture(const AZStd::string& cultureCode)
    {
        m_activeCulture = cultureCode;

        // Scan the asset catalog for all LexiconAssemblyAsset products and queue
        // a load for the first one whose cultures list contains cultureCode.
        AZ::Data::AssetCatalogRequests::AssetEnumerationCB enumCb =
            [this](const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& info)
            {
                if (info.m_assetType != azrtti_typeid<Heathen::LexiconAssemblyAsset>())
                {
                    return;
                }

                // Load and inspect — OnAssetReady will assign to m_activeAsset if cultures match
                AZ::Data::Asset<Heathen::LexiconAssemblyAsset> candidate =
                    AZ::Data::AssetManager::Instance().GetAsset<Heathen::LexiconAssemblyAsset>(
                        assetId, AZ::Data::AssetLoadBehavior::QueueLoad);

                AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
            };

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequests::EnumerateAssets, nullptr, enumCb, nullptr);
    }

    AZStd::string FoundationLocalisationSystemComponent::GetActiveCulture() const
    {
        return m_activeCulture;
    }

    void FoundationLocalisationSystemComponent::SetDefaultCulture(const AZStd::string& cultureCode)
    {
        m_defaultCulture = cultureCode;

        AZ::Data::AssetCatalogRequests::AssetEnumerationCB enumCb =
            [this](const AZ::Data::AssetId assetId, const AZ::Data::AssetInfo& info)
            {
                if (info.m_assetType != azrtti_typeid<Heathen::LexiconAssemblyAsset>())
                {
                    return;
                }

                AZ::Data::Asset<Heathen::LexiconAssemblyAsset> candidate =
                    AZ::Data::AssetManager::Instance().GetAsset<Heathen::LexiconAssemblyAsset>(
                        assetId, AZ::Data::AssetLoadBehavior::QueueLoad);

                AZ::Data::AssetBus::MultiHandler::BusConnect(assetId);
            };

        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequests::EnumerateAssets, nullptr, enumCb, nullptr);
    }

    AZStd::vector<AZStd::string> FoundationLocalisationSystemComponent::GetAvailableLexiconIds() const
    {
        AZStd::vector<AZStd::string> ids;
        ids.reserve(m_allLoadedAssets.size());
        for (const auto& asset : m_allLoadedAssets)
        {
            if (asset.IsReady() && !asset->m_assetId.empty())
            {
                ids.push_back(asset->m_assetId);
            }
        }
        return ids;
    }

    AZStd::string FoundationLocalisationSystemComponent::GetLexiconDisplayName(
        const AZStd::string& assetId) const
    {
        const AZStd::string key = "Language." + assetId;
        const AZ::u64 hash = xxHash::xxHashFunctions::Hash64(key, 0);

        // Try active culture first, fall back to default
        if (m_activeAsset.IsReady())
        {
            AZStd::string name = m_activeAsset->FindString(hash);
            if (!name.empty())
                return name;
        }
        if (m_defaultAsset.IsReady())
        {
            AZStd::string name = m_defaultAsset->FindString(hash);
            if (!name.empty())
                return name;
        }

        // No entry found — return the raw assetId as a readable fallback
        return assetId;
    }

    void FoundationLocalisationSystemComponent::OnAssetReady(
        AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        auto lexicon = azrtti_cast<Heathen::LexiconAssemblyAsset*>(asset.Get());
        if (!lexicon)
        {
            return;
        }

        // Check whether this lexicon services the active culture
        const auto& cultures = lexicon->m_cultures;
        const bool servesActive  = AZStd::find(cultures.begin(), cultures.end(), m_activeCulture)  != cultures.end();
        const bool servesDefault = AZStd::find(cultures.begin(), cultures.end(), m_defaultCulture) != cultures.end();

        if (servesActive && !m_activeAsset.IsReady())
        {
            m_activeAsset = asset;
        }
        else if (servesDefault && !m_defaultAsset.IsReady())
        {
            m_defaultAsset = asset;
        }

        // Track every successfully loaded lexicon for GetAvailableLexiconIds()
        const bool alreadyTracked = AZStd::find_if(
            m_allLoadedAssets.begin(), m_allLoadedAssets.end(),
            [&asset](const AZ::Data::Asset<Heathen::LexiconAssemblyAsset>& a)
            {
                return a.GetId() == asset.GetId();
            }) != m_allLoadedAssets.end();

        if (!alreadyTracked)
        {
            m_allLoadedAssets.push_back(asset);
        }

        AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());
    }

    void FoundationLocalisationSystemComponent::OnAssetError(
        AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        AZ_Warning("FoundationLocalisation", false,
            "Failed to load LexiconAssemblyAsset: %s",
            asset.GetHint().c_str());

        AZ::Data::AssetBus::MultiHandler::BusDisconnect(asset.GetId());
    }

    ////////////////////////////////////////////////////////////////////////

} // namespace FoundationLocalisation

////////////////////////////////////////////////////////////////////////
// Console commands
// Defined at file scope (anonymous namespace) so the command names are
// unqualified in the console (e.g. "helex_hash", not "FoundationLocalisation::helex_hash").
////////////////////////////////////////////////////////////////////////

namespace
{
    using FLReqs = FoundationLocalisation::FoundationLocalisationRequests;
    using FLBus  = FoundationLocalisation::FoundationLocalisationRequestBus;

    void helex_hash(const AZ::ConsoleCommandContainer& args)
    {
        if (args.empty())
        {
            AZ_Printf("HeLex", "Usage: helex_hash <dot-path-key>");
            return;
        }
        const AZStd::string key(args[0].data(), args[0].size());
        const AZ::u64 hash = xxHash::xxHashFunctions::Hash64(key, 0);
        AZ_Printf("HeLex", "helex_hash('%s') = %llu (0x%016llX)",
            key.c_str(),
            static_cast<unsigned long long>(hash),
            static_cast<unsigned long long>(hash));
    }
    AZ_CONSOLEFREEFUNC(helex_hash, AZ::ConsoleFunctorFlags::Null,
        "Hashes a dot-path key with XXH3_64bits (seed 0) and prints the u64 result.");

    void helex_resolve(const AZ::ConsoleCommandContainer& args)
    {
        if (args.empty())
        {
            AZ_Printf("HeLex", "Usage: helex_resolve <dot-path-key>");
            return;
        }
        const AZStd::string path(args[0].data(), args[0].size());
        const AZ::u64 hash = xxHash::xxHashFunctions::Hash64(path, 0);

        // Try string first
        AZStd::string strResult;
        FLBus::BroadcastResult(strResult,
            static_cast<AZStd::string(FLReqs::*)(AZ::u64)>(&FLReqs::ResolveString), hash);

        if (!strResult.empty())
        {
            AZ_Printf("HeLex", "helex_resolve('%s') = \"%s\"", path.c_str(), strResult.c_str());
            return;
        }

        // Try asset ID
        AZ::Uuid assetId;
        FLBus::BroadcastResult(assetId,
            static_cast<AZ::Uuid(FLReqs::*)(AZ::u64)>(&FLReqs::ResolveAssetId), hash);

        if (!assetId.IsNull())
        {
            AZ_Printf("HeLex", "helex_resolve('%s') = asset %s",
                path.c_str(), assetId.ToString<AZStd::string>().c_str());
            return;
        }

        AZ_Printf("HeLex", "helex_resolve('%s') = (not found — is a culture loaded?)", path.c_str());
    }
    AZ_CONSOLEFREEFUNC(helex_resolve, AZ::ConsoleFunctorFlags::Null,
        "Resolves a dot-path key against the active culture and prints the string or asset UUID result.");

    void helex_setculture(const AZ::ConsoleCommandContainer& args)
    {
        if (args.empty())
        {
            AZStd::string active;
            FLBus::BroadcastResult(active, &FLReqs::GetActiveCulture);
            AZ_Printf("HeLex", "Usage: helex_setculture <culture-code>  (active: '%s')",
                active.empty() ? "(none)" : active.c_str());
            return;
        }
        const AZStd::string code(args[0].data(), args[0].size());
        FLBus::Broadcast(&FLReqs::LoadCulture, code);
        AZ_Printf("HeLex", "helex_setculture: loading culture '%s'…", code.c_str());
    }
    AZ_CONSOLEFREEFUNC(helex_setculture, AZ::ConsoleFunctorFlags::Null,
        "Loads a culture by IETF code (e.g. en-GB) and sets it as the active localisation culture.");

} // anonymous namespace
