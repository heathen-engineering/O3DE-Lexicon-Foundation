
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/string/string.h>

namespace FoundationLocalisation
{
    class FoundationLocalisationRequests
    {
    public:
        AZ_RTTI(FoundationLocalisationRequests, "{3FC78351-1AA3-43E1-AAA8-8B05FAF2E48C}");
        virtual ~FoundationLocalisationRequests() = default;

        ///<summary>
        /// Resolve a pre-hashed key to a string in the active lexicon.
        /// Falls back to the default lexicon on miss. Returns empty string if
        /// the key is not found in either.
        ///</summary>
        virtual AZStd::string ResolveString(AZ::u64 key) = 0;

        ///<summary>
        /// Hash the dot-path then resolve. Convenience overload; prefer the
        /// u64 overload in hot paths where the hash is already cached.
        ///</summary>
        virtual AZStd::string ResolveString(const AZStd::string& path) = 0;

        ///<summary>
        /// Resolve a pre-hashed key to an asset UUID in the active lexicon.
        /// Falls back to the default lexicon on miss. Returns a null UUID if
        /// the key is not found in either.
        ///</summary>
        virtual AZ::Uuid ResolveAssetId(AZ::u64 key) = 0;

        ///<summary>
        /// Hash the dot-path then resolve. Convenience overload.
        ///</summary>
        virtual AZ::Uuid ResolveAssetId(const AZStd::string& path) = 0;

        ///<summary>
        /// Load the lexicon that services the given culture code (e.g. "en-GB").
        /// The asset is located by scanning registered LexiconAssemblyAssets for
        /// one whose m_cultures list contains the requested code.
        /// Triggers an async asset load; resolution returns empty/null until complete.
        ///</summary>
        virtual void LoadCulture(const AZStd::string& cultureCode) = 0;

        ///<summary>
        /// Returns the culture code currently active (e.g. "en-GB"), or empty
        /// string if no culture has been loaded.
        ///</summary>
        virtual AZStd::string GetActiveCulture() const = 0;

        ///<summary>
        /// Set the culture code used as the fallback when a key is not found
        /// in the active lexicon. Triggers a load if not already loaded.
        ///</summary>
        virtual void SetDefaultCulture(const AZStd::string& cultureCode) = 0;

        ///<summary>
        /// Returns the assetId of every LexiconAssemblyAsset that has been loaded
        /// (active + default + any explicitly loaded). Used to enumerate available
        /// languages for e.g. an Options Menu.
        ///</summary>
        virtual AZStd::vector<AZStd::string> GetAvailableLexiconIds() const = 0;

        ///<summary>
        /// Returns the display name for the given assetId resolved via the reserved
        /// "Language.<assetId>" key in the active (then default) culture.
        /// Falls back to the assetId itself if no entry is found.
        /// e.g. GetLexiconDisplayName("French_Standard") → "Français"
        ///</summary>
        virtual AZStd::string GetLexiconDisplayName(const AZStd::string& assetId) const = 0;
    };
    
    class FoundationLocalisationBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using FoundationLocalisationRequestBus = AZ::EBus<FoundationLocalisationRequests, FoundationLocalisationBusTraits>;
    using FoundationLocalisationInterface = AZ::Interface<FoundationLocalisationRequests>;

} // namespace FoundationLocalisation
