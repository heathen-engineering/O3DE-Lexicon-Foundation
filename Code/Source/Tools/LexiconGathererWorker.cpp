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

#include "LexiconGathererWorker.h"

#include <AzCore/IO/FileIO.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
AZ_POP_DISABLE_WARNING

#include <algorithm>

namespace FoundationLocalisation
{
    ////////////////////////////////////////////////////////////////////////
    // Helpers (file-scope only)

    /// Derives a dot-path key from a literal value string.
    /// e.g. "New Game" → "Literal.NewGame"
    static AZStd::string MakeProposedKey(const AZStd::string& value)
    {
        AZStd::string key = "Literal.";
        bool capitalizeNext = true;
        for (unsigned char c : value)
        {
            if (c == ' ' || c == '\t' || c == '-' || c == '_')
            {
                capitalizeNext = true;
                continue;
            }
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
            {
                if (capitalizeNext && c >= 'a' && c <= 'z')
                    c = static_cast<unsigned char>(c - 'a' + 'A');
                key += static_cast<char>(c);
                capitalizeNext = false;
            }
            // Strip other characters (punctuation, unicode, etc.)
        }
        if (key == "Literal.")
            key += "Empty";
        return key;
    }

    /// Recurse into a rapidjson Value, collecting Literal-mode Lexicon objects.
    /// An object qualifies when it has m_mode == 1 (Literal) and m_keyOrValue as string.
    static void ScanValue(
        const rapidjson::Value& val,
        const AZStd::string& filePath,
        AZStd::vector<GatheredLiteral>& results)
    {
        if (val.IsArray())
        {
            for (const auto& elem : val.GetArray())
                ScanValue(elem, filePath, results);
            return;
        }

        if (!val.IsObject())
            return;

        // Check for Literal-mode Lexicon pattern
        const bool hasMode = val.HasMember("m_mode") && val["m_mode"].IsInt();
        const bool isLiteral = hasMode && val["m_mode"].GetInt() == 1;
        const bool hasValue  = val.HasMember("m_keyOrValue") && val["m_keyOrValue"].IsString();

        if (isLiteral && hasValue)
        {
            const AZStd::string value(
                val["m_keyOrValue"].GetString(),
                val["m_keyOrValue"].GetStringLength());

            GatheredLiteral lit;
            lit.m_filePath     = filePath;
            lit.m_currentValue = value;
            lit.m_proposedKey  = MakeProposedKey(value);
            results.push_back(AZStd::move(lit));
            // Don't recurse into matched objects — their children aren't Lexicon fields
            return;
        }

        // Recurse into all members
        for (auto it = val.MemberBegin(); it != val.MemberEnd(); ++it)
            ScanValue(it->value, filePath, results);
    }

    /// Recurse and patch every Literal-mode object whose m_keyOrValue == currentValue.
    static void PatchValue(
        rapidjson::Value& val,
        rapidjson::Document::AllocatorType& alloc,
        const AZStd::string& currentValue,
        const AZStd::string& newKey)
    {
        if (val.IsArray())
        {
            for (auto& elem : val.GetArray())
                PatchValue(elem, alloc, currentValue, newKey);
            return;
        }

        if (!val.IsObject())
            return;

        const bool isLiteral = val.HasMember("m_mode") && val["m_mode"].IsInt()
                             && val["m_mode"].GetInt() == 1;
        const bool hasValue  = val.HasMember("m_keyOrValue") && val["m_keyOrValue"].IsString();

        if (isLiteral && hasValue)
        {
            const AZStd::string v(
                val["m_keyOrValue"].GetString(),
                val["m_keyOrValue"].GetStringLength());

            if (v == currentValue)
            {
                val["m_mode"].SetInt(0); // LexiconLocMode::Localised
                val["m_keyOrValue"].SetString(
                    newKey.c_str(),
                    static_cast<rapidjson::SizeType>(newKey.size()),
                    alloc);
                return; // Don't recurse into a patched object
            }
        }

        for (auto it = val.MemberBegin(); it != val.MemberEnd(); ++it)
            PatchValue(it->value, alloc, currentValue, newKey);
    }

    ////////////////////////////////////////////////////////////////////////
    // LexiconGathererWorker

    AZStd::vector<GatheredLiteral> LexiconGathererWorker::ScanPrefabs()
    {
        AZStd::vector<GatheredLiteral> results;

        const AZ::IO::FixedMaxPath projectPath = AZ::Utils::GetProjectPath();
        if (projectPath.empty())
            return results;

        auto* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
            return results;

        // Collect all .prefab files recursively
        AZStd::vector<AZStd::string> prefabFiles;
        AZStd::function<void(const AZStd::string&)> collect =
            [&](const AZStd::string& dir)
            {
                fileIO->FindFiles(dir.c_str(), "*.prefab",
                    [&prefabFiles](const char* path) -> bool
                    {
                        prefabFiles.emplace_back(path);
                        return true;
                    });
                fileIO->FindFiles(dir.c_str(), "*",
                    [&collect](const char* path) -> bool
                    {
                        if (AZ::IO::FileIOBase::GetInstance()->IsDirectory(path))
                            collect(path);
                        return true;
                    });
            };

        collect(AZStd::string(projectPath.c_str()));

        // Parse each prefab and collect Literal-mode fields
        for (const AZStd::string& filePath : prefabFiles)
        {
            AZ::IO::HandleType handle = AZ::IO::InvalidHandle;
            if (fileIO->Open(filePath.c_str(),
                    AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary,
                    handle) != AZ::IO::ResultCode::Success)
                continue;

            AZ::u64 size = 0;
            fileIO->Size(handle, size);
            AZStd::vector<char> buf(static_cast<size_t>(size) + 1, '\0');
            fileIO->Read(handle, buf.data(), size);
            fileIO->Close(handle);

            rapidjson::Document doc;
            doc.ParseInsitu(buf.data());
            if (doc.HasParseError() || !doc.IsObject())
                continue;

            ScanValue(doc, filePath, results);
        }

        // Deduplicate: same (file, value) pair → single row in the dialog
        std::sort(results.begin(), results.end(),
            [](const GatheredLiteral& a, const GatheredLiteral& b)
            {
                if (a.m_filePath != b.m_filePath)
                    return a.m_filePath < b.m_filePath;
                return a.m_currentValue < b.m_currentValue;
            });

        results.erase(
            std::unique(results.begin(), results.end(),
                [](const GatheredLiteral& a, const GatheredLiteral& b)
                {
                    return a.m_filePath == b.m_filePath
                        && a.m_currentValue == b.m_currentValue;
                }),
            results.end());

        return results;
    }

    bool LexiconGathererWorker::WriteToHelex(
        const AZStd::string& helexPath,
        const AZStd::vector<GatheredLiteral>& entries)
    {
        auto* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
            return false;

        rapidjson::Document doc;
        doc.SetObject();
        auto& alloc = doc.GetAllocator();

        // Load existing .helex if it exists; start fresh if parse fails
        AZ::IO::HandleType handle = AZ::IO::InvalidHandle;
        if (fileIO->Open(helexPath.c_str(),
                AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary,
                handle) == AZ::IO::ResultCode::Success)
        {
            AZ::u64 size = 0;
            fileIO->Size(handle, size);
            AZStd::vector<char> buf(static_cast<size_t>(size) + 1, '\0');
            fileIO->Read(handle, buf.data(), size);
            fileIO->Close(handle);
            doc.ParseInsitu(buf.data());
            if (doc.HasParseError() || !doc.IsObject())
                doc.SetObject();
        }

        // Ensure "cultures" and "entries" members exist
        if (!doc.HasMember("cultures"))
        {
            rapidjson::Value cultures(rapidjson::kArrayType);
            doc.AddMember("cultures", cultures, alloc);
        }
        if (!doc.HasMember("entries"))
        {
            rapidjson::Value entriesObj(rapidjson::kObjectType);
            doc.AddMember("entries", entriesObj, alloc);
        }

        rapidjson::Value& entriesObj = doc["entries"];

        for (const GatheredLiteral& lit : entries)
        {
            if (lit.m_proposedKey.empty())
                continue;
            // Never overwrite an existing key
            if (entriesObj.HasMember(lit.m_proposedKey.c_str()))
                continue;

            rapidjson::Value key(
                lit.m_proposedKey.c_str(),
                static_cast<rapidjson::SizeType>(lit.m_proposedKey.size()), alloc);
            rapidjson::Value val(
                lit.m_currentValue.c_str(),
                static_cast<rapidjson::SizeType>(lit.m_currentValue.size()), alloc);
            entriesObj.AddMember(key, val, alloc);
        }

        // Serialise to pretty JSON
        rapidjson::StringBuffer sb;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
        doc.Accept(writer);

        AZ::IO::HandleType writeHandle = AZ::IO::InvalidHandle;
        if (fileIO->Open(helexPath.c_str(),
                AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText,
                writeHandle) != AZ::IO::ResultCode::Success)
            return false;

        fileIO->Write(writeHandle, sb.GetString(), sb.GetSize());
        fileIO->Close(writeHandle);
        return true;
    }

    void LexiconGathererWorker::PatchPrefabs(const AZStd::vector<GatheredLiteral>& entries)
    {
        auto* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
            return;

        // Group entries by source file so we only re-parse each prefab once
        AZStd::unordered_map<AZStd::string, AZStd::vector<const GatheredLiteral*>> byFile;
        for (const GatheredLiteral& lit : entries)
            byFile[lit.m_filePath].push_back(&lit);

        for (auto& pair : byFile)
        {
            const AZStd::string& filePath = pair.first;
            const AZStd::vector<const GatheredLiteral*>& lits = pair.second;

            // Read file (non-insitu so we can re-serialise)
            AZ::IO::HandleType handle = AZ::IO::InvalidHandle;
            if (fileIO->Open(filePath.c_str(),
                    AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary,
                    handle) != AZ::IO::ResultCode::Success)
                continue;

            AZ::u64 size = 0;
            fileIO->Size(handle, size);
            AZStd::vector<char> buf(static_cast<size_t>(size) + 1, '\0');
            fileIO->Read(handle, buf.data(), size);
            fileIO->Close(handle);

            rapidjson::Document doc;
            doc.Parse(buf.data()); // non-insitu — keep the buffer for error reporting
            if (doc.HasParseError() || !doc.IsObject())
            {
                AZ_Warning("LexiconGatherer", false,
                    "Could not parse '%s' for patching — skipping.", filePath.c_str());
                continue;
            }

            for (const GatheredLiteral* lit : lits)
            {
                if (!lit->m_proposedKey.empty())
                    PatchValue(doc, doc.GetAllocator(), lit->m_currentValue, lit->m_proposedKey);
            }

            // Write back as pretty JSON
            rapidjson::StringBuffer sb;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);
            doc.Accept(writer);

            AZ::IO::HandleType writeHandle = AZ::IO::InvalidHandle;
            if (fileIO->Open(filePath.c_str(),
                    AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeText,
                    writeHandle) != AZ::IO::ResultCode::Success)
            {
                AZ_Warning("LexiconGatherer", false,
                    "Could not open '%s' for writing — prefab not patched.", filePath.c_str());
                continue;
            }

            fileIO->Write(writeHandle, sb.GetString(), sb.GetSize());
            fileIO->Close(writeHandle);
        }
    }

    ////////////////////////////////////////////////////////////////////////

} // namespace FoundationLocalisation
