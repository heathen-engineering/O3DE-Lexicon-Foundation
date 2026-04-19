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

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>

#include <FoundationLocalisation/FoundationLocalisationTypeIds.h>

namespace FoundationLocalisation
{
    ///<summary>
    /// Converts .helex source files into binary LexiconAssemblyAsset (.lexicon) products.
    ///
    /// Each .helex file is processed into exactly one .lexicon per enabled platform.
    /// The builder:
    ///   - Hashes every dot-path key with XXH3_64bits (seed 0)
    ///   - Appends string values as UTF-8 null-terminated bytes into a blob
    ///   - Appends UUID asset-reference values as raw 16-byte AZ::Uuid into the blob
    ///   - Emits a ProductDependency for every UUID entry (prevents asset stripping)
    ///   - Sorts the index ascending by key hash for O(log n) runtime lookup
    ///   - Serialises the LexiconAssemblyAsset to binary via AZ::Utils::SaveObjectToFile
    ///</summary>
    class LexiconAssemblyAssetBuilder
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_RTTI(LexiconAssemblyAssetBuilder, Heathen::LexiconAssemblyAssetBuilderTypeId);

        static constexpr const char* BuilderName = "HeLex Lexicon Builder";
        static constexpr const char* FilePattern = "*.helex";
        static constexpr const char* JobKey      = "HeLex Compile";

        LexiconAssemblyAssetBuilder()  = default;
        ~LexiconAssemblyAssetBuilder() = default;

        /// Register this builder with the AssetBuilderSDK. Call once from the
        /// Editor system component's Activate().
        void RegisterBuilder();

        void CreateJobs(
            const AssetBuilderSDK::CreateJobsRequest&  request,
                  AssetBuilderSDK::CreateJobsResponse& response);

        void ProcessJob(
            const AssetBuilderSDK::ProcessJobRequest&  request,
                  AssetBuilderSDK::ProcessJobResponse& response);

        // AssetBuilderSDK::AssetBuilderCommandBus::Handler
        void ShutDown() override;

    private:
        bool m_isShuttingDown = false;
    };

} // namespace FoundationLocalisation
