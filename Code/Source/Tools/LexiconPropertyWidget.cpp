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

#include "LexiconPropertyWidget.h"
#include "LexiconKeyPickerDialog.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>

namespace FoundationLocalisation
{
    LexiconPropertyWidget::LexiconPropertyWidget(QWidget* parent)
        : QWidget(parent)
    {
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(2);

        // ---- Mode button with pop-up menu ----
        m_modeButton = new QToolButton(this);
        m_modeButton->setToolTip(
            "Localised — key resolved via active lexicon\n"
            "Literal   — raw value, no lookup\n"
            "Invariant — raw value, excluded from the Gatherer");
        m_modeButton->setPopupMode(QToolButton::InstantPopup);
        m_modeButton->setMinimumWidth(36);

        QMenu* modeMenu = new QMenu(m_modeButton);
        modeMenu->addAction("Localised", this, &LexiconPropertyWidget::OnSetLocalised);
        modeMenu->addAction("Literal",   this, &LexiconPropertyWidget::OnSetLiteral);
        modeMenu->addAction("Invariant", this, &LexiconPropertyWidget::OnSetInvariant);
        m_modeButton->setMenu(modeMenu);

        // ---- Value / key line edit ----
        m_valueEdit = new QLineEdit(this);
        m_valueEdit->setPlaceholderText("(empty)");
        connect(m_valueEdit, &QLineEdit::editingFinished,
                this, &LexiconPropertyWidget::OnLineEditEditingFinished);

        // ---- Key-picker trigger (Localised mode only) ----
        m_pickButton = new QPushButton("…", this);
        m_pickButton->setToolTip("Open key picker");
        m_pickButton->setMaximumWidth(28);
        connect(m_pickButton, &QPushButton::clicked,
                this, &LexiconPropertyWidget::OnPickClicked);

        layout->addWidget(m_modeButton);
        layout->addWidget(m_valueEdit, 1);
        layout->addWidget(m_pickButton);

        UpdateDisplay();
    }

    void LexiconPropertyWidget::SetValue(const AZStd::string& text, Heathen::LexiconLocMode mode)
    {
        m_mode = mode;
        m_valueEdit->setText(QString::fromUtf8(text.c_str()));
        UpdateDisplay();
    }

    AZStd::string LexiconPropertyWidget::GetText() const
    {
        return AZStd::string(m_valueEdit->text().toUtf8().constData());
    }

    Heathen::LexiconLocMode LexiconPropertyWidget::GetMode() const
    {
        return m_mode;
    }

    // ---- Private slots ----

    void LexiconPropertyWidget::OnSetLocalised()
    {
        m_mode = Heathen::LexiconLocMode::Localised;
        UpdateDisplay();
        emit ValueChangedByUser();
    }

    void LexiconPropertyWidget::OnSetLiteral()
    {
        m_mode = Heathen::LexiconLocMode::Literal;
        UpdateDisplay();
        emit ValueChangedByUser();
    }

    void LexiconPropertyWidget::OnSetInvariant()
    {
        m_mode = Heathen::LexiconLocMode::Invariant;
        UpdateDisplay();
        emit ValueChangedByUser();
    }

    void LexiconPropertyWidget::OnLineEditEditingFinished()
    {
        emit ValueChangedByUser();
    }

    void LexiconPropertyWidget::OnPickClicked()
    {
        LexiconKeyPickerDialog dialog(this);
        dialog.SetCurrentKey(AZStd::string(m_valueEdit->text().toUtf8().constData()));

        if (dialog.exec() == QDialog::Accepted)
        {
            const AZStd::string selected = dialog.GetSelectedKey();
            if (!selected.empty())
            {
                m_mode = Heathen::LexiconLocMode::Localised;
                m_valueEdit->setText(QString::fromUtf8(selected.c_str()));
                UpdateDisplay();
                emit ValueChangedByUser();
            }
        }
    }

    // ---- Private helpers ----

    void LexiconPropertyWidget::UpdateDisplay()
    {
        const bool isLocalised = (m_mode == Heathen::LexiconLocMode::Localised);

        switch (m_mode)
        {
            case Heathen::LexiconLocMode::Localised:
                m_modeButton->setText("Loc");
                m_valueEdit->setPlaceholderText("dot.path.key");
                break;
            case Heathen::LexiconLocMode::Literal:
                m_modeButton->setText("Lit");
                m_valueEdit->setPlaceholderText("(raw value)");
                break;
            case Heathen::LexiconLocMode::Invariant:
                m_modeButton->setText("Inv");
                m_valueEdit->setPlaceholderText("(invariant)");
                break;
        }

        m_valueEdit->setReadOnly(isLocalised);
        m_valueEdit->setStyleSheet(isLocalised
            ? "color: grey; background: #1a1a1a;"
            : "");
        m_pickButton->setVisible(isLocalised);
    }

} // namespace FoundationLocalisation
