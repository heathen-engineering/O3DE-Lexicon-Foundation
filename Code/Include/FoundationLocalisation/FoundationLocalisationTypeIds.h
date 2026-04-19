
#pragma once

namespace FoundationLocalisation
{
    // System Component TypeIds
    inline constexpr const char* FoundationLocalisationSystemComponentTypeId = "{A530CB1A-6E16-4D03-9BE8-3CBEBC93D2D0}";
    inline constexpr const char* FoundationLocalisationEditorSystemComponentTypeId = "{A218C432-4D82-414A-B045-0B8607BDDCE2}";

    // Module derived classes TypeIds
    inline constexpr const char* FoundationLocalisationModuleInterfaceTypeId = "{435B8DA1-629A-41EF-B780-98E5971DEB9B}";
    inline constexpr const char* FoundationLocalisationModuleTypeId = "{C12D7F9F-D28B-4BA9-ADC5-B9D2F480A47E}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* FoundationLocalisationEditorModuleTypeId = FoundationLocalisationModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* FoundationLocalisationRequestsTypeId = "{610F593C-FB41-4212-AC8C-3291568ADDD3}";

} // namespace FoundationLocalisation

namespace Heathen
{
    // LexiconAssemblyAsset TypeIds
    inline constexpr const char* LexiconAssemblyAssetTypeId  = "{4A7B3C91-E2D5-4F08-B6A1-937C285E1D40}";
    inline constexpr const char* LexiconAssemblyEntryTypeId  = "{9F2E4B71-A3C6-4D1F-8E05-B4712938CE5A}";

    // Localisation data type TypeIds
    inline constexpr const char* LexiconTextTypeId  = "{A1E3C5B7-29D4-4F8E-B06A-3718C92DE541}";
    inline constexpr const char* LexiconSoundTypeId = "{B72F4A9C-51E3-4D16-A8CF-4829D30E7B62}";
    inline constexpr const char* LexiconAssetTypeId = "{C83056BD-72F4-4E27-B9D0-5930E41F8C73}";

    // LexiconLocMode TypeId (used by ReflectLexiconLocMode)
    inline constexpr const char* LexiconLocModeTypeId = "{E7F8091A-2B3C-4D5E-6F70-8192A3B4C5D6}";

    // Asset builder TypeId
    inline constexpr const char* LexiconAssemblyAssetBuilderTypeId = "{D5E6F7A8-B9C0-4D1E-8F2A-3B4C5D6E7F80}";

} // namespace Heathen
