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

#include <QWidget>

// AZ headers contain macros defined inside namespace AZ that confuse Qt MOC's
// simple namespace tracker, causing it to think subsequent namespaces are nested
// inside AZ. Guard the full includes so MOC only sees forward declarations.
#ifndef Q_MOC_RUN
#  include <AzCore/std/string/string.h>
#  include <FoundationLocalisation/LexiconLocMode.h>
#else
namespace AZStd { class string; }
namespace Heathen { enum class LexiconLocMode : unsigned char; }
#endif

class QToolButton;
class QLineEdit;
class QPushButton;

namespace FoundationLocalisation
{
    ///<summary>
    /// Shared row widget used by all three Lexicon property handlers
    /// (LexiconText, LexiconSound, LexiconAsset).
    ///
    /// Layout:  [Mode ▾]  [key / value line edit          ]  [Pick…]
    ///
    /// Mode button — opens a QMenu to switch between Localised / Literal /
    ///               Invariant. Label reflects the current mode.
    ///
    /// Line edit  — read-only in Localised mode (key); editable in
    ///              Literal / Invariant mode (raw value or UUID string).
    ///
    /// Pick button — visible only in Localised mode; opens the
    ///               LexiconKeyPickerDialog (Phase 3.3).
    ///
    /// Emits ValueChangedByUser whenever the user modifies mode or value.
    /// The owning property handler connects this signal to
    /// PropertyEditorGUIMessages::RequestWrite.
    ///</summary>
    class LexiconPropertyWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit LexiconPropertyWidget(QWidget* parent = nullptr);

        ///<summary>Sets the displayed text and mode without emitting any signal.</summary>
        void SetValue(const AZStd::string& text, Heathen::LexiconLocMode mode);

        AZStd::string           GetText() const;
        Heathen::LexiconLocMode GetMode() const;

    signals:
        ///<summary>Fired whenever the user changes mode or edits the value field.</summary>
        void ValueChangedByUser();

    private slots:
        void OnSetLocalised();
        void OnSetLiteral();
        void OnSetInvariant();
        void OnLineEditEditingFinished();
        void OnPickClicked();

    private:
        void UpdateDisplay();

        QToolButton* m_modeButton = nullptr;
        QLineEdit*   m_valueEdit  = nullptr;
        QPushButton* m_pickButton = nullptr;

        Heathen::LexiconLocMode m_mode = Heathen::LexiconLocMode::Literal;
    };

} // namespace FoundationLocalisation
