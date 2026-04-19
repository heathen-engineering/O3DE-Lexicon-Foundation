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

#include "LexiconCsvInterop.h"

#include <QFile>
#include <QFileInfo>
#include <QTextCodec>
#include <QTextStream>

namespace FoundationLocalisation
{
    ////////////////////////////////////////////////////////////////////////////
    // RFC 4180 helpers

    QString LexiconCsvInterop::QuoteField(const QString& value)
    {
        // Wrap in double-quotes if the value contains a comma, double-quote, or newline.
        if (!value.contains(QLatin1Char(','))  &&
            !value.contains(QLatin1Char('"'))  &&
            !value.contains(QLatin1Char('\n')) &&
            !value.contains(QLatin1Char('\r')))
        {
            return value;
        }
        // Escape embedded double-quotes by doubling them.
        QString escaped = value;
        escaped.replace(QLatin1String("\""), QLatin1String("\"\""));
        return QLatin1Char('"') + escaped + QLatin1Char('"');
    }

    QStringList LexiconCsvInterop::ParseRow(const QString& line)
    {
        // RFC 4180 parser — handles quoted fields spanning a single line.
        // Multi-line quoted fields are not supported (helex values are never multi-line).
        QStringList fields;
        QString current;
        bool inQuotes = false;

        for (int i = 0; i < line.size(); ++i)
        {
            const QChar ch = line[i];

            if (inQuotes)
            {
                if (ch == QLatin1Char('"'))
                {
                    // Peek ahead: doubled quote → literal quote character
                    if (i + 1 < line.size() && line[i + 1] == QLatin1Char('"'))
                    {
                        current += QLatin1Char('"');
                        ++i;
                    }
                    else
                    {
                        inQuotes = false;
                    }
                }
                else
                {
                    current += ch;
                }
            }
            else
            {
                if (ch == QLatin1Char('"'))
                {
                    inQuotes = true;
                }
                else if (ch == QLatin1Char(','))
                {
                    fields.append(current);
                    current.clear();
                }
                else
                {
                    current += ch;
                }
            }
        }
        fields.append(current);
        return fields;
    }

    QString LexiconCsvInterop::StatusLabel(const LexiconEntry& entry,
                                            bool hasSource,
                                            bool hasActive)
    {
        if (hasSource && hasActive)
        {
            if (entry.isMissing)   return QStringLiteral("missing");
            if (entry.isOrphan)    return QStringLiteral("orphan");
            if (entry.isDuplicate) return QStringLiteral("duplicate");
            if (entry.isEmpty)     return QStringLiteral("empty");
            return QStringLiteral("translated");
        }
        if (hasActive && entry.activeValue.isEmpty())
            return QStringLiteral("empty");
        return QStringLiteral("ok");
    }

    ////////////////////////////////////////////////////////////////////////////
    // Export — Single

    QString LexiconCsvInterop::ExportSingle(
        const QString&        csvFilePath,
        const LexiconEntryMap& entries,
        const QString&        workingFilePath)
    {
        QFile file(csvFilePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
            return QString("Could not open file for writing:\n%1").arg(csvFilePath);

        QTextStream out(&file);
        out.setCodec("UTF-8");

        const QString workingFileName = QFileInfo(workingFilePath).fileName();

        // Row 0 — human-readable header
        out << QuoteField(QStringLiteral("Key"))
            << QLatin1Char(',')
            << QuoteField(QStringLiteral("Value"))
            << QLatin1Char('\n');

        // Row 1 — file identity
        out << QuoteField(QStringLiteral(""))   // key column placeholder
            << QLatin1Char(',')
            << QuoteField(workingFileName)
            << QLatin1Char('\n');

        // Data rows
        for (auto it = entries.cbegin(); it != entries.cend(); ++it)
        {
            out << QuoteField(it.key())
                << QLatin1Char(',')
                << QuoteField(it.value().activeValue)
                << QLatin1Char('\n');
        }

        file.close();
        return {};
    }

    ////////////////////////////////////////////////////////////////////////////
    // Export — Multi

    QString LexiconCsvInterop::ExportMulti(
        const QString&                  csvFilePath,
        const QStringList&              filePaths,
        const QVector<LexiconEntryMap>& entryMaps)
    {
        if (filePaths.size() != entryMaps.size())
            return QStringLiteral("filePaths and entryMaps must have the same length.");

        QFile file(csvFilePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
            return QString("Could not open file for writing:\n%1").arg(csvFilePath);

        QTextStream out(&file);
        out.setCodec("UTF-8");

        // Collect the union of all keys, preserving sort order.
        QMap<QString, bool> allKeysMap;
        for (const LexiconEntryMap& map : entryMaps)
            for (auto it = map.cbegin(); it != map.cend(); ++it)
                allKeysMap.insert(it.key(), true);

        const QStringList allKeys = allKeysMap.keys();

        // Row 0 — human-readable headers
        out << QuoteField(QStringLiteral("Key"));
        for (const QString& path : filePaths)
            out << QLatin1Char(',') << QuoteField(QFileInfo(path).baseName());
        out << QLatin1Char('\n');

        // Row 1 — file names (identity row)
        out << QuoteField(QStringLiteral(""));  // key column placeholder
        for (const QString& path : filePaths)
            out << QLatin1Char(',') << QuoteField(QFileInfo(path).fileName());
        out << QLatin1Char('\n');

        // Data rows
        for (const QString& key : allKeys)
        {
            out << QuoteField(key);
            for (const LexiconEntryMap& map : entryMaps)
            {
                const QString value = map.contains(key) ? map.value(key).activeValue : QString{};
                out << QLatin1Char(',') << QuoteField(value);
            }
            out << QLatin1Char('\n');
        }

        file.close();
        return {};
    }

    ////////////////////////////////////////////////////////////////////////////
    // Import — Single

    LexiconCsvInterop::SingleImportResult LexiconCsvInterop::ParseSingle(
        const QString& csvFilePath)
    {
        SingleImportResult result;

        QFile file(csvFilePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            result.errors << QString("Could not open file:\n%1").arg(csvFilePath);
            return result;
        }

        QTextStream in(&file);
        in.setCodec("UTF-8");

        int lineIndex = 0;
        while (!in.atEnd())
        {
            const QString line = in.readLine();
            const QStringList fields = ParseRow(line);

            if (lineIndex == 0)
            {
                // Row 0 — header, skip
                ++lineIndex;
                continue;
            }
            if (lineIndex == 1)
            {
                // Row 1 — identity row; Col 1 = suggested file name
                if (fields.size() >= 2)
                    result.suggestedFilePath = fields[1].trimmed();
                ++lineIndex;
                continue;
            }

            // Data row
            if (fields.size() < 2)
            {
                if (!line.trimmed().isEmpty())
                    result.errors << QString("Row %1 skipped — fewer than 2 columns.").arg(lineIndex + 1);
                ++lineIndex;
                continue;
            }

            const QString key   = fields[0].trimmed();
            const QString value = fields[1];

            if (key.isEmpty())
            {
                ++lineIndex;
                continue;
            }

            result.pairs.insert(key, value);
            ++lineIndex;
        }

        file.close();
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Import — Multi

    LexiconCsvInterop::MultiImportResult LexiconCsvInterop::ParseMulti(
        const QString& csvFilePath)
    {
        MultiImportResult result;

        QFile file(csvFilePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            result.errors << QString("Could not open file:\n%1").arg(csvFilePath);
            return result;
        }

        QTextStream in(&file);
        in.setCodec("UTF-8");

        int lineIndex = 0;

        while (!in.atEnd())
        {
            const QString line = in.readLine();
            const QStringList fields = ParseRow(line);

            if (lineIndex == 0)
            {
                // Row 0 — header, skip
                ++lineIndex;
                continue;
            }
            if (lineIndex == 1)
            {
                // Row 1 — identity row: Col 1..N = file names
                // Col 0 is the key column placeholder (ignored).
                const int valueCols = fields.size() - 1;
                result.columns.resize(valueCols);
                for (int c = 0; c < valueCols; ++c)
                    result.columns[c].filePath = fields[c + 1].trimmed();
                ++lineIndex;
                continue;
            }

            // Data row — Col 0 = key, Col 1..N = values
            if (fields.isEmpty())
            {
                ++lineIndex;
                continue;
            }

            const QString key = fields[0].trimmed();
            if (key.isEmpty())
            {
                ++lineIndex;
                continue;
            }

            for (int c = 0; c < result.columns.size(); ++c)
            {
                const int fieldIdx = c + 1;
                const QString value = (fieldIdx < fields.size()) ? fields[fieldIdx] : QString{};
                result.columns[c].pairs.insert(key, value);
            }

            ++lineIndex;
        }

        file.close();
        return result;
    }

} // namespace FoundationLocalisation
