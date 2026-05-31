
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

#include <FoundationLocalisation/LexiconHintType.h>

namespace FoundationLocalisation
{
    class FoundationLocalisationRequests
    {
    public:
        AZ_RTTI(FoundationLocalisationRequests, "{3FC78351-1AA3-43E1-AAA8-8B05FAF2E48C}");
        virtual ~FoundationLocalisationRequests() = default;

        ///<summary>
        /// Resolve a pre-hashed key to a string in the active lexicon.
        /// Resolution order: active culture → base language of active (e.g. fr-CA → fr)
        /// → default culture → base language of default.
        /// Returns empty string if the key is not found at any level.
        ///</summary>
        virtual AZStd::string ResolveString(AZ::u64 key) = 0;

        ///<summary>
        /// Hash the dot-path then resolve. Convenience overload; prefer the
        /// u64 overload in hot paths where the hash is already cached.
        ///</summary>
        virtual AZStd::string ResolveString(const AZStd::string& path) = 0;

        ///<summary>
        /// Resolve a pre-hashed key to an asset UUID in the active lexicon.
        /// Applies the same four-level fallback as ResolveString.
        /// Returns a null UUID if the key is not found at any level.
        ///</summary>
        virtual AZ::Uuid ResolveAssetId(AZ::u64 key) = 0;

        ///<summary>
        /// Hash the dot-path then resolve. Convenience overload.
        ///</summary>
        virtual AZ::Uuid ResolveAssetId(const AZStd::string& path) = 0;

        ///<summary>
        /// Load the lexicon that services the given culture code (e.g. "en-GB").
        /// Scans all registered LexiconAssemblyAssets for one whose m_cultures
        /// list contains the requested code. Triggers an async asset load;
        /// resolution returns empty/null until the load completes.
        /// Fires OnCultureChanged on LexiconNotificationBus when done.
        ///</summary>
        virtual void LoadCulture(const AZStd::string& cultureCode) = 0;

        ///<summary>
        /// Alias for LoadCulture — matches Unity API naming (UseCulture).
        ///</summary>
        virtual void UseCulture(const AZStd::string& cultureCode)
        {
            LoadCulture(cultureCode);
        }

        ///<summary>
        /// Returns the culture code currently active (e.g. "en-GB"), or empty
        /// string if no culture has been loaded.
        ///</summary>
        virtual AZStd::string GetActiveCulture() const = 0;

        ///<summary>
        /// Set the culture code used as the fallback when a key is not found
        /// in the active lexicon. Triggers a load if not already loaded.
        /// Fires OnDefaultCultureChanged on LexiconNotificationBus.
        ///</summary>
        virtual void SetDefaultCulture(const AZStd::string& cultureCode) = 0;

        ///<summary>
        /// Returns every culture code served by any currently loaded
        /// LexiconAssemblyAsset. Use this to populate a language-selection menu.
        /// Example: ["en-GB", "en-IE", "fr", "fr-CA", "de"]
        ///</summary>
        virtual AZStd::vector<AZStd::string> GetMappedCultureCodes() const = 0;

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

        ///<summary>
        /// Returns the hint type encoded in the binary entry for the given pre-hashed key.
        /// Returns None if the key is not found in either active or default culture.
        /// Cost: one binary search (same as ResolveString/ResolveAssetId).
        ///</summary>
        virtual LexiconHintType GetEntryHintType(AZ::u64 key) const = 0;
    };

    class FoundationLocalisationBusTraits
        : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };

    using FoundationLocalisationRequestBus = AZ::EBus<FoundationLocalisationRequests, FoundationLocalisationBusTraits>;
    using FoundationLocalisationInterface = AZ::Interface<FoundationLocalisationRequests>;

} // namespace FoundationLocalisation

// =============================================================================
// LexiconNotificationBus
// Broadcast when the active or default culture changes.
// UI components (e.g. LexiconTextComponent) connect here to refresh displayed
// text when the player switches language at runtime.
// =============================================================================
namespace FoundationLocalisation
{
    class LexiconNotifications : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        /// Called after the active culture has changed and the new lexicon asset
        /// is ready. 'cultureCode' is the IETF code that is now active.
        virtual void OnCultureChanged(const AZStd::string& cultureCode) { (void)cultureCode; }

        /// Called after the default culture has changed.
        virtual void OnDefaultCultureChanged(const AZStd::string& cultureCode) { (void)cultureCode; }
    };

    using LexiconNotificationBus = AZ::EBus<LexiconNotifications>;

} // namespace FoundationLocalisation
