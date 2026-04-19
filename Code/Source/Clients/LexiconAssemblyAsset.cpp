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

#include <FoundationLocalisation/LexiconAssemblyAsset.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <cstring>

namespace Heathen
{
    void LexiconAssemblyAsset::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<Entry>()
                ->Version(0)
                ->Field("KeyHash",    &Entry::m_keyHash)
                ->Field("DataOffset", &Entry::m_dataOffset)
                ->Field("DataSize",   &Entry::m_dataSize);

            serialize->Class<LexiconAssemblyAsset, AZ::Data::AssetData>()
                ->Version(1)
                ->Field("AssetId",    &LexiconAssemblyAsset::m_assetId)
                ->Field("Cultures",   &LexiconAssemblyAsset::m_cultures)
                ->Field("Index",      &LexiconAssemblyAsset::m_index)
                ->Field("BufferBlob", &LexiconAssemblyAsset::m_bufferBlob);
        }
    }

    AZStd::string LexiconAssemblyAsset::FindString(AZ::u64 key) const
    {
        const Entry* e = BinarySearch(key);
        if (!e || e->IsAssetId())
        {
            return {};
        }

        const AZ::u32 size = e->DataSize();
        if (size == 0 || e->m_dataOffset + size > m_bufferBlob.size())
        {
            return {};
        }

        const char* ptr = reinterpret_cast<const char*>(m_bufferBlob.data() + e->m_dataOffset);
        // size includes the null terminator — construct without it
        return AZStd::string(ptr, size - 1);
    }

    AZ::Uuid LexiconAssemblyAsset::FindAssetId(AZ::u64 key) const
    {
        const Entry* e = BinarySearch(key);
        if (!e || !e->IsAssetId())
        {
            return AZ::Uuid::CreateNull();
        }

        AZ_Assert(e->DataSize() == sizeof(AZ::Uuid),
            "LexiconAssemblyAsset: asset ID entry at offset %u has unexpected size %u",
            e->m_dataOffset, e->DataSize());

        if (e->m_dataOffset + sizeof(AZ::Uuid) > m_bufferBlob.size())
        {
            return AZ::Uuid::CreateNull();
        }

        AZ::Uuid result;
        memcpy(&result, m_bufferBlob.data() + e->m_dataOffset, sizeof(AZ::Uuid));
        return result;
    }

    const LexiconAssemblyAsset::Entry* LexiconAssemblyAsset::BinarySearch(AZ::u64 key) const
    {
        if (m_index.empty())
        {
            return nullptr;
        }

        AZ::s64 lo = 0;
        AZ::s64 hi = static_cast<AZ::s64>(m_index.size()) - 1;

        while (lo <= hi)
        {
            const AZ::s64 mid = lo + (hi - lo) / 2;
            const AZ::u64 midKey = m_index[static_cast<size_t>(mid)].m_keyHash;

            if (midKey == key)  return &m_index[static_cast<size_t>(mid)];
            if (midKey < key)   lo = mid + 1;
            else                hi = mid - 1;
        }

        return nullptr;
    }

} // namespace Heathen
