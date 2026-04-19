
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/std/string/string.h>

#include <FoundationLocalisation/FoundationLocalisationBus.h>
#include <FoundationLocalisation/LexiconAssemblyAsset.h>

namespace FoundationLocalisation
{
    class FoundationLocalisationSystemComponent
        : public AZ::Component
        , protected FoundationLocalisationRequestBus::Handler
        , public AZ::Data::AssetBus::MultiHandler
    {
    public:
        AZ_COMPONENT_DECL(FoundationLocalisationSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        FoundationLocalisationSystemComponent();
        ~FoundationLocalisationSystemComponent();

    private:
        AZ::Data::Asset<Heathen::LexiconAssemblyAsset> m_activeAsset;
        AZ::Data::Asset<Heathen::LexiconAssemblyAsset> m_defaultAsset;
        AZStd::string m_activeCulture;
        AZStd::string m_defaultCulture;

        /// All lexicons loaded since Activate — used by GetAvailableLexiconIds().
        AZStd::vector<AZ::Data::Asset<Heathen::LexiconAssemblyAsset>> m_allLoadedAssets;

    protected:
        ////////////////////////////////////////////////////////////////////////
        // FoundationLocalisationRequestBus interface implementation
        AZStd::string ResolveString(AZ::u64 key) override;
        AZStd::string ResolveString(const AZStd::string& path) override;
        AZ::Uuid      ResolveAssetId(AZ::u64 key) override;
        AZ::Uuid      ResolveAssetId(const AZStd::string& path) override;
        void          LoadCulture(const AZStd::string& cultureCode) override;
        AZStd::string GetActiveCulture() const override;
        void          SetDefaultCulture(const AZStd::string& cultureCode) override;
        AZStd::vector<AZStd::string> GetAvailableLexiconIds() const override;
        AZStd::string GetLexiconDisplayName(const AZStd::string& assetId) const override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Data::AssetBus::MultiHandler
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetError(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    };

} // namespace FoundationLocalisation
