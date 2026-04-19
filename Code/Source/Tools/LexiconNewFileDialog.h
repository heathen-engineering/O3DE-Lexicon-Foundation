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

#include <QDialog>
#include <QString>

class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;

namespace FoundationLocalisation
{
    ///<summary>
    /// Modal dialog for creating a new .helex file.
    ///
    /// Collects:
    ///   assetId       — stable internal name (e.g. "French_Standard"), used as
    ///                   the "assetId" JSON field and for Language.* keys
    ///   displayName   — value pre-filled for Language.<assetId> in this file
    ///   cultureCodes  — comma-separated codes (e.g. "fr-FR, fr-CA"), written
    ///                   to the "cultures" JSON array
    ///   savePath      — absolute path where the .helex file will be written
    ///
    /// On accept, call FilePath(), AssetId(), DisplayName(), CultureCodes() to
    /// retrieve the confirmed values. The file is written by LexiconToolWindow
    /// after the dialog closes, not by the dialog itself.
    ///</summary>
    class LexiconNewFileDialog : public QDialog
    {
        Q_OBJECT

    public:
        ///<param name="defaultDir">Starting directory for the Browse button. Pass the
        ///project source root so the dialog opens there rather than at the engine path.</param>
        explicit LexiconNewFileDialog(const QString& defaultDir = QString(),
                                      QWidget* parent = nullptr);

        QString FilePath()     const;
        QString AssetId()      const;
        QString DisplayName()  const;
        QString CultureCodes() const;

    private slots:
        void OnBrowse();
        void OnValidate();

    private:
        QString           m_defaultDir;
        QLineEdit*        m_assetIdEdit      = nullptr;
        QLineEdit*        m_displayNameEdit  = nullptr;
        QLineEdit*        m_cultureCodesEdit = nullptr;
        QLineEdit*        m_savePathEdit     = nullptr;
        QPushButton*      m_browseBtn        = nullptr;
        QLabel*           m_errorLabel       = nullptr;
        QDialogButtonBox* m_buttons          = nullptr;
    };

} // namespace FoundationLocalisation
