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

#include "LexiconAssemblyAssetBuilder.h"

#include <FoundationLocalisation/LexiconAssemblyAsset.h>

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

#include <algorithm>

#include <xxHash/xxHashFunctions.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
AZ_POP_DISABLE_WARNING

namespace FoundationLocalisation
{
    void LexiconAssemblyAssetBuilder::RegisterBuilder()
    {
        AssetBuilderSDK::AssetBuilderDesc desc;
        desc.m_name       = BuilderName;
        desc.m_busId      = azrtti_typeid<LexiconAssemblyAssetBuilder>();
        desc.m_version    = 1;
        desc.m_builderType = AssetBuilderSDK::AssetBuilderDesc::AssetBuilderType::External;

        desc.m_patterns.emplace_back(
            AssetBuilderSDK::AssetBuilderPattern(FilePattern,
                AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));

        desc.m_createJobFunction = AZStd::bind(
            &LexiconAssemblyAssetBuilder::CreateJobs, this,
            AZStd::placeholders::_1, AZStd::placeholders::_2);

        desc.m_processJobFunction = AZStd::bind(
            &LexiconAssemblyAssetBuilder::ProcessJob, this,
            AZStd::placeholders::_1, AZStd::placeholders::_2);

        BusConnect(desc.m_busId);
        AssetBuilderSDK::AssetBuilderBus::Broadcast(
            &AssetBuilderSDK::AssetBuilderBus::Events::RegisterBuilderInformation, desc);
    }

    void LexiconAssemblyAssetBuilder::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void LexiconAssemblyAssetBuilder::CreateJobs(
        const AssetBuilderSDK::CreateJobsRequest&  request,
              AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        for (const AssetBuilderSDK::PlatformInfo& platform : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor job;
            job.m_jobKey = JobKey;
            job.SetPlatformIdentifier(platform.m_identifier.c_str());
            response.m_createJobOutputs.push_back(AZStd::move(job));
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void LexiconAssemblyAssetBuilder::ProcessJob(
        const AssetBuilderSDK::ProcessJobRequest&  request,
              AssetBuilderSDK::ProcessJobResponse& response)
    {
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;

        if (m_isShuttingDown)
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        // ----------------------------------------------------------------
        // 1. Read source file
        // ----------------------------------------------------------------
        AZ::IO::FileIOStream fileStream(
            request.m_fullPath.c_str(),
            AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary);

        if (!fileStream.IsOpen())
        {
            AZ_Error("LexiconAssemblyAssetBuilder", false,
                "Failed to open source file: %s", request.m_fullPath.c_str());
            return;
        }

        const AZ::IO::SizeType fileSize = fileStream.GetLength();
        AZStd::vector<char> content(fileSize + 1, '\0');
        fileStream.Read(fileSize, content.data());
        fileStream.Close();

        // ----------------------------------------------------------------
        // 2. Parse JSON
        // ----------------------------------------------------------------
        rapidjson::Document doc;
        doc.ParseInsitu(content.data());

        if (doc.HasParseError())
        {
            AZ_Error("LexiconAssemblyAssetBuilder", false,
                "%s: JSON parse error at offset %zu: %s",
                request.m_fullPath.c_str(),
                doc.GetErrorOffset(),
                rapidjson::GetParseError_En(doc.GetParseError()));
            return;
        }

        if (!doc.IsObject())
        {
            AZ_Error("LexiconAssemblyAssetBuilder", false,
                "%s: root element must be a JSON object.", request.m_fullPath.c_str());
            return;
        }

        // ----------------------------------------------------------------
        // 3. Build LexiconAssemblyAsset
        // ----------------------------------------------------------------
        Heathen::LexiconAssemblyAsset asset;

        // assetId — use JSON field if present, otherwise fall back to the source file stem
        if (doc.HasMember("assetId") && doc["assetId"].IsString()
            && doc["assetId"].GetStringLength() > 0)
        {
            asset.m_assetId = AZStd::string(
                doc["assetId"].GetString(), doc["assetId"].GetStringLength());
        }
        else
        {
            asset.m_assetId = AZStd::string(
                AZ::IO::Path(request.m_sourceFile).Stem().Native());
        }

        // cultures array
        if (doc.HasMember("cultures") && doc["cultures"].IsArray())
        {
            for (const auto& c : doc["cultures"].GetArray())
            {
                if (c.IsString())
                {
                    asset.m_cultures.emplace_back(c.GetString());
                }
            }
        }

        if (asset.m_cultures.empty())
        {
            AZ_Warning("LexiconAssemblyAssetBuilder", false,
                "%s: no cultures defined — this asset will never be selected by LoadCulture().",
                request.m_fullPath.c_str());
        }

        // entries object
        AssetBuilderSDK::JobProduct product;
        product.m_productSubID = 0;
        product.m_productFileName = AZ::IO::Path(request.m_fullPath).Stem().String() + ".lexicon";

        if (doc.HasMember("entries") && doc["entries"].IsObject())
        {
            for (auto it = doc["entries"].MemberBegin(); it != doc["entries"].MemberEnd(); ++it)
            {
                if (!it->name.IsString())
                {
                    continue;
                }

                const AZStd::string key(it->name.GetString(), it->name.GetStringLength());
                const AZ::u64 keyHash = xxHash::xxHashFunctions::Hash64(key, 0);

                Heathen::LexiconAssemblyAsset::Entry entry;
                entry.m_keyHash    = keyHash;
                entry.m_dataOffset = static_cast<AZ::u32>(asset.m_bufferBlob.size());

                if (it->value.IsString())
                {
                    // ---- String entry ----
                    const char*  str    = it->value.GetString();
                    const AZ::u32 len   = static_cast<AZ::u32>(it->value.GetStringLength());
                    const AZ::u32 size  = len + 1; // include null terminator

                    entry.m_dataSize = Heathen::LexiconAssemblyAsset::Entry::TypeFlagString | size;

                    asset.m_bufferBlob.insert(
                        asset.m_bufferBlob.end(),
                        reinterpret_cast<const AZ::u8*>(str),
                        reinterpret_cast<const AZ::u8*>(str) + len);
                    asset.m_bufferBlob.push_back(0); // null terminator
                }
                else if (it->value.IsObject()
                    && it->value.HasMember("uuid")
                    && it->value["uuid"].IsString())
                {
                    // ---- Asset ID entry ----
                    const AZStd::string uuidStr(
                        it->value["uuid"].GetString(),
                        it->value["uuid"].GetStringLength());

                    const AZ::Uuid uuid = AZ::Uuid::CreateString(uuidStr.c_str(), uuidStr.size());

                    if (uuid.IsNull())
                    {
                        AZ_Warning("LexiconAssemblyAssetBuilder", false,
                            "%s: entry '%s' has an invalid UUID string '%s' — skipping.",
                            request.m_fullPath.c_str(), key.c_str(), uuidStr.c_str());
                        continue;
                    }

                    constexpr AZ::u32 uuidSize = static_cast<AZ::u32>(sizeof(AZ::Uuid));
                    entry.m_dataSize =
                        Heathen::LexiconAssemblyAsset::Entry::TypeFlagAsset | uuidSize;

                    const AZ::u8* uuidBytes = reinterpret_cast<const AZ::u8*>(&uuid);
                    asset.m_bufferBlob.insert(
                        asset.m_bufferBlob.end(), uuidBytes, uuidBytes + uuidSize);

                    // Declare product dependency so the referenced asset is not stripped
                    product.m_dependencies.emplace_back(
                        AssetBuilderSDK::ProductDependency(
                            AZ::Data::AssetId(uuid, 0),
                            AZ::Data::ProductDependencyInfo::CreateFlags(
                                AZ::Data::AssetLoadBehavior::NoLoad)));
                }
                else
                {
                    AZ_Warning("LexiconAssemblyAssetBuilder", false,
                        "%s: entry '%s' has an unrecognised value type — skipping.",
                        request.m_fullPath.c_str(), key.c_str());
                    continue;
                }

                asset.m_index.push_back(entry);
            }
        }

        // ----------------------------------------------------------------
        // 4. Sort index ascending by key hash
        // ----------------------------------------------------------------
        std::sort(asset.m_index.begin(), asset.m_index.end(),
            [](const Heathen::LexiconAssemblyAsset::Entry& a,
               const Heathen::LexiconAssemblyAsset::Entry& b)
            {
                return a.m_keyHash < b.m_keyHash;
            });

        // ----------------------------------------------------------------
        // 5. Serialise to .lexicon
        // ----------------------------------------------------------------
        AZ::IO::Path outputPath(request.m_tempDirPath);
        outputPath /= product.m_productFileName;

        if (!AZ::Utils::SaveObjectToFile(
                outputPath.String(), AZ::DataStream::ST_BINARY, &asset))
        {
            AZ_Error("LexiconAssemblyAssetBuilder", false,
                "Failed to serialise asset to: %s", outputPath.c_str());
            return;
        }

        // ----------------------------------------------------------------
        // 6. Report product
        // ----------------------------------------------------------------
        product.m_productAssetType = azrtti_typeid<Heathen::LexiconAssemblyAsset>();
        response.m_outputProducts.push_back(AZStd::move(product));
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

} // namespace FoundationLocalisation
