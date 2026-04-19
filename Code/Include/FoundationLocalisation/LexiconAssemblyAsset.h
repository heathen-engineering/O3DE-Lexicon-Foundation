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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <FoundationLocalisation/FoundationLocalisationTypeIds.h>

namespace Heathen
{
    ///<summary>
    /// Runtime binary representation of a .helex localisation file.
    ///
    /// Contains a sorted index of key-hash → blob-offset entries and a flat
    /// buffer blob holding the raw string and UUID data. Lookups are O(log n)
    /// binary search on the index.
    ///
    /// Each asset declares the culture codes it services (m_cultures). A single
    /// asset may cover multiple cultures (e.g. "pt-BR" and "es-419").
    ///
    /// Produced from .helex source files by the LexiconAssemblyAssetBuilder.
    /// Do not modify the index or blob at runtime.
    ///</summary>
    class LexiconAssemblyAsset : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(LexiconAssemblyAsset, LexiconAssemblyAssetTypeId, AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(LexiconAssemblyAsset, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        ///<summary>
        /// A single entry in the sorted lookup index.
        ///
        /// Memory layout (16 bytes, naturally aligned):
        ///   m_keyHash    [8] — XXH3_64bits of the dot-path key, seed 0
        ///   m_dataOffset [4] — byte offset into m_bufferBlob
        ///   m_dataSize   [4] — byte count of data; high bit encodes type:
        ///                       0 = String  (UTF-8, null-terminated, size includes '\0')
        ///                       1 = AssetId (raw AZ::Uuid bytes, always 16 bytes)
        ///</summary>
        struct Entry
        {
            AZ_TYPE_INFO(Entry, LexiconAssemblyEntryTypeId);

            static constexpr AZ::u32 TypeFlagMask   = 0x80000000u;
            static constexpr AZ::u32 TypeFlagString = 0x00000000u;
            static constexpr AZ::u32 TypeFlagAsset  = 0x80000000u;
            static constexpr AZ::u32 SizeMask       = 0x7FFFFFFFu;

            AZ::u64 m_keyHash    = 0;
            AZ::u32 m_dataOffset = 0;
            AZ::u32 m_dataSize   = 0;

            bool     IsAssetId() const { return (m_dataSize & TypeFlagMask) != 0; }
            AZ::u32  DataSize()  const { return  m_dataSize & SizeMask; }
        };

        ///<summary>
        /// Look up a string value by pre-computed key hash.
        /// Returns an empty string if the key is not found or the entry is not a string.
        ///</summary>
        AZStd::string FindString(AZ::u64 key) const;

        ///<summary>
        /// Look up an asset UUID by pre-computed key hash.
        /// Returns a null AZ::Uuid if the key is not found or the entry is not an asset reference.
        ///</summary>
        AZ::Uuid FindAssetId(AZ::u64 key) const;

        /// Stable internal identifier set from the .helex "assetId" field
        /// (e.g. "French_Standard"). Used by GetAvailableLexiconIds() and
        /// the Language.* self-naming key convention.
        AZStd::string m_assetId;

        /// Culture codes this asset services (e.g. "en-GB", "en-IE").
        AZStd::vector<AZStd::string> m_cultures;

        /// Lookup index, sorted ascending by m_keyHash.
        AZStd::vector<Entry> m_index;

        /// Raw data blob: UTF-8 strings (null-terminated) and 16-byte AZ::Uuid values.
        AZStd::vector<AZ::u8> m_bufferBlob;

    private:
        const Entry* BinarySearch(AZ::u64 key) const;
    };

} // namespace Heathen
