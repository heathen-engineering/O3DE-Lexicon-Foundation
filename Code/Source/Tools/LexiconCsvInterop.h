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

#include "LexiconEntryMap.h"

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

namespace FoundationLocalisation
{
    ///<summary>
    /// CSV interop for the Lexicon Tool (Phase 6.7).
    ///
    /// All CSV files use the same fixed structure:
    ///
    ///   Row 0 — human-readable column headers  (ignored on import)
    ///   Row 1 — .helex file names              (identity row; used by Multi import)
    ///   Row 2+ — data: Col 0 = key, Col 1..N = value per lexicon
    ///
    /// Export modes:
    ///   Single — key + one value column (the Working lexicon).
    ///   Multi  — key + one value column per discovered lexicon.
    ///
    /// Import modes:
    ///   Multi  (default) — reads every value column; maps each to the .helex file
    ///                      named in Row 1.  Unknown file names create a new lexicon
    ///                      next to default.helex.
    ///   Single           — reads only Column 1 (first value column); caller supplies
    ///                      the target file path.  The Row 1 name is returned as a
    ///                      suggested target path so the UI can pre-fill the picker.
    ///
    /// CSV encoding follows RFC 4180: values containing commas, double-quotes, or
    /// newlines are wrapped in double-quotes; any embedded double-quote is doubled.
    ///</summary>
    class LexiconCsvInterop
    {
    public:
        // ---- Export --------------------------------------------------------

        ///<summary>
        /// Writes a Single-mode CSV: key + one value column for workingPath.
        ///
        /// Returns an error string on failure, empty string on success.
        ///</summary>
        static QString ExportSingle(
            const QString&        csvFilePath,
            const LexiconEntryMap& entries,
            const QString&        workingFilePath);

        ///<summary>
        /// Writes a Multi-mode CSV: key + one value column per file in filePaths.
        ///
        /// entryMaps[i] must correspond to filePaths[i].
        /// Returns an error string on failure, empty string on success.
        ///</summary>
        static QString ExportMulti(
            const QString&               csvFilePath,
            const QStringList&           filePaths,
            const QVector<LexiconEntryMap>& entryMaps);

        // ---- Import --------------------------------------------------------

        ///<summary>
        /// Result of a single-column import parse.
        ///</summary>
        struct SingleImportResult
        {
            QString              suggestedFilePath; ///< Row 1 Col 1 value — use as default in picker
            QMap<QString,QString> pairs;            ///< key → value for every data row
            QStringList          errors;            ///< parse warnings / skipped rows
        };

        ///<summary>
        /// Parses a CSV for Single-mode import.
        /// Reads Col 0 (key) + Col 1 (value) from Row 2 onwards.
        /// Row 1 Col 1 is returned as suggestedFilePath.
        ///</summary>
        static SingleImportResult ParseSingle(const QString& csvFilePath);

        ///<summary>
        /// Result of a multi-column import parse.
        ///</summary>
        struct MultiImportResult
        {
            /// Per-column data: index matches the value columns (Col 1, Col 2, …).
            struct Column
            {
                QString              filePath; ///< From Row 1 — may be absolute or basename
                QMap<QString,QString> pairs;   ///< key → value
            };
            QVector<Column> columns;
            QStringList     errors;   ///< parse warnings / skipped rows
        };

        ///<summary>
        /// Parses a CSV for Multi-mode import.
        /// All value columns are returned; Row 1 names become filePath on each column.
        ///</summary>
        static MultiImportResult ParseMulti(const QString& csvFilePath);

    private:
        // RFC 4180 helpers
        static QString     QuoteField(const QString& value);
        static QStringList ParseRow(const QString& line);
        static QString     StatusLabel(const LexiconEntry& entry, bool hasSource, bool hasActive);
    };

} // namespace FoundationLocalisation
