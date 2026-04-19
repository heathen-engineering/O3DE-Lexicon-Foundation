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

#include "LexiconNewFileDialog.h"

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace FoundationLocalisation
{
    LexiconNewFileDialog::LexiconNewFileDialog(const QString& defaultDir, QWidget* parent)
        : QDialog(parent)
        , m_defaultDir(defaultDir)
    {
        setWindowTitle("New Lexicon File");
        setMinimumWidth(420);

        auto* root = new QVBoxLayout(this);
        root->setSpacing(8);
        root->setContentsMargins(12, 12, 12, 12);

        // ---- Form fields ------------------------------------------------
        auto* form = new QFormLayout();
        form->setLabelAlignment(Qt::AlignRight);
        form->setSpacing(6);

        m_assetIdEdit = new QLineEdit(this);
        m_assetIdEdit->setPlaceholderText("e.g. French_Standard");
        m_assetIdEdit->setToolTip(
            "Stable internal identifier for this lexicon.\n"
            "Used as the JSON 'assetId' field and for Language.* keys.\n"
            "No spaces — use underscores.");
        form->addRow("Asset ID:", m_assetIdEdit);

        m_displayNameEdit = new QLineEdit(this);
        m_displayNameEdit->setPlaceholderText("e.g. Français");
        m_displayNameEdit->setToolTip(
            "Display name of this language in its own culture.\n"
            "Written as Language.<assetId> inside this file.");
        form->addRow("Display Name:", m_displayNameEdit);

        m_cultureCodesEdit = new QLineEdit(this);
        m_cultureCodesEdit->setPlaceholderText("e.g. fr-FR, fr-CA, fr-BE");
        m_cultureCodesEdit->setToolTip(
            "Comma-separated IETF culture codes serviced by this lexicon.\n"
            "The runtime uses these to select the correct lexicon at startup.");
        form->addRow("Culture Codes:", m_cultureCodesEdit);

        // Save path row
        auto* pathWidget  = new QWidget(this);
        auto* pathLayout  = new QHBoxLayout(pathWidget);
        pathLayout->setContentsMargins(0, 0, 0, 0);
        pathLayout->setSpacing(4);

        m_savePathEdit = new QLineEdit(pathWidget);
        m_savePathEdit->setPlaceholderText("Choose a location…");
        m_savePathEdit->setReadOnly(true);

        m_browseBtn = new QPushButton("Browse…", pathWidget);
        connect(m_browseBtn, &QPushButton::clicked, this, &LexiconNewFileDialog::OnBrowse);

        pathLayout->addWidget(m_savePathEdit, 1);
        pathLayout->addWidget(m_browseBtn);

        form->addRow("Save Path:", pathWidget);

        root->addLayout(form);

        // ---- Error label ------------------------------------------------
        m_errorLabel = new QLabel(this);
        m_errorLabel->setStyleSheet("color: #e05050;");
        m_errorLabel->setWordWrap(true);
        m_errorLabel->hide();
        root->addWidget(m_errorLabel);

        // ---- Buttons ----------------------------------------------------
        m_buttons = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(m_buttons, &QDialogButtonBox::accepted, this, &LexiconNewFileDialog::OnValidate);
        connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        root->addWidget(m_buttons);

        setLayout(root);
    }

    QString LexiconNewFileDialog::FilePath()     const { return m_savePathEdit->text().trimmed(); }
    QString LexiconNewFileDialog::AssetId()      const { return m_assetIdEdit->text().trimmed(); }
    QString LexiconNewFileDialog::DisplayName()  const { return m_displayNameEdit->text().trimmed(); }
    QString LexiconNewFileDialog::CultureCodes() const { return m_cultureCodesEdit->text().trimmed(); }

    void LexiconNewFileDialog::OnBrowse()
    {
        QString path = QFileDialog::getSaveFileName(
            this, "Save New Lexicon As", m_defaultDir,
            "HeLex Lexicon (*.helex);;All Files (*)");

        if (!path.isEmpty())
        {
            // Qt on Linux doesn't always auto-append the extension
            if (!path.endsWith(QLatin1String(".helex"), Qt::CaseInsensitive))
                path += QLatin1String(".helex");
            m_savePathEdit->setText(path);
        }
    }

    void LexiconNewFileDialog::OnValidate()
    {
        m_errorLabel->hide();

        const QString assetId  = AssetId();
        const QString savePath = FilePath();

        if (assetId.isEmpty())
        {
            m_errorLabel->setText("Asset ID is required.");
            m_errorLabel->show();
            m_assetIdEdit->setFocus();
            return;
        }

        // Reject spaces and most special characters in assetId
        static const QRegularExpression validId(R"([A-Za-z0-9_\-]+)");
        if (!validId.match(assetId).hasMatch() ||
            validId.match(assetId).captured() != assetId)
        {
            m_errorLabel->setText("Asset ID may only contain letters, digits, underscores, and hyphens.");
            m_errorLabel->show();
            m_assetIdEdit->setFocus();
            return;
        }

        if (savePath.isEmpty())
        {
            m_errorLabel->setText("Please choose a save location.");
            m_errorLabel->show();
            return;
        }

        accept();
    }

} // namespace FoundationLocalisation

#include <moc_LexiconNewFileDialog.cpp>
