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

#include "LexiconPropertyHandler.h"

#include <AzCore/Math/Uuid.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

namespace FoundationLocalisation
{
    // ---- Shared CreateGUI helper ----
    // All three handlers produce the same widget type; only the signal connection
    // differs (it always points back to the same widget for RequestWrite).

    static LexiconPropertyWidget* MakeWidget(QWidget* parent)
    {
        auto* widget = new LexiconPropertyWidget(parent);
        QObject::connect(widget, &LexiconPropertyWidget::ValueChangedByUser,
            widget,
            [widget]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &AzToolsFramework::PropertyEditorGUIMessages::RequestWrite,
                    widget);
            });
        return widget;
    }

    ////////////////////////////////////////////////////////////////////////
    // LexiconTextPropertyHandler

    QWidget* LexiconTextPropertyHandler::CreateGUI(QWidget* parent)
    {
        return MakeWidget(parent);
    }

    void LexiconTextPropertyHandler::WriteGUIValuesIntoProperty(
        [[maybe_unused]] size_t index,
        LexiconPropertyWidget*   widget,
        Heathen::LexiconText&    property,
        [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        property.m_mode       = widget->GetMode();
        property.m_keyOrValue = widget->GetText();
        property.InvalidateHash();
    }

    bool LexiconTextPropertyHandler::ReadValuesIntoGUI(
        [[maybe_unused]] size_t index,
        LexiconPropertyWidget*        widget,
        const Heathen::LexiconText&   property,
        [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        widget->SetValue(property.m_keyOrValue, property.m_mode);
        return false;
    }

    ////////////////////////////////////////////////////////////////////////
    // LexiconSoundPropertyHandler

    QWidget* LexiconSoundPropertyHandler::CreateGUI(QWidget* parent)
    {
        return MakeWidget(parent);
    }

    void LexiconSoundPropertyHandler::WriteGUIValuesIntoProperty(
        [[maybe_unused]] size_t index,
        LexiconPropertyWidget*   widget,
        Heathen::LexiconSound&   property,
        [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        property.m_mode = widget->GetMode();

        if (property.m_mode == Heathen::LexiconLocMode::Localised)
        {
            property.m_keyOrValue = widget->GetText();
            property.InvalidateHash();
        }
        else
        {
            // Attempt to parse the text field as a UUID asset reference
            const AZStd::string text = widget->GetText();
            const AZ::Uuid parsed = AZ::Uuid::CreateString(text.c_str(), text.size());
            if (!parsed.IsNull())
            {
                property.m_literalAssetId = parsed;
            }
        }
    }

    bool LexiconSoundPropertyHandler::ReadValuesIntoGUI(
        [[maybe_unused]] size_t index,
        LexiconPropertyWidget*        widget,
        const Heathen::LexiconSound&  property,
        [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        AZStd::string display = property.m_keyOrValue;
        if (property.m_mode != Heathen::LexiconLocMode::Localised
            && !property.m_literalAssetId.IsNull())
        {
            display = property.m_literalAssetId.ToString<AZStd::string>();
        }
        widget->SetValue(display, property.m_mode);
        return false;
    }

    ////////////////////////////////////////////////////////////////////////
    // LexiconAssetPropertyHandler

    QWidget* LexiconAssetPropertyHandler::CreateGUI(QWidget* parent)
    {
        return MakeWidget(parent);
    }

    void LexiconAssetPropertyHandler::WriteGUIValuesIntoProperty(
        [[maybe_unused]] size_t index,
        LexiconPropertyWidget*   widget,
        Heathen::LexiconAsset&   property,
        [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        property.m_mode = widget->GetMode();

        if (property.m_mode == Heathen::LexiconLocMode::Localised)
        {
            property.m_keyOrValue = widget->GetText();
            property.InvalidateHash();
        }
        else
        {
            const AZStd::string text = widget->GetText();
            const AZ::Uuid parsed = AZ::Uuid::CreateString(text.c_str(), text.size());
            if (!parsed.IsNull())
            {
                property.m_literalAssetId = parsed;
            }
        }
    }

    bool LexiconAssetPropertyHandler::ReadValuesIntoGUI(
        [[maybe_unused]] size_t index,
        LexiconPropertyWidget*       widget,
        const Heathen::LexiconAsset& property,
        [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        AZStd::string display = property.m_keyOrValue;
        if (property.m_mode != Heathen::LexiconLocMode::Localised
            && !property.m_literalAssetId.IsNull())
        {
            display = property.m_literalAssetId.ToString<AZStd::string>();
        }
        widget->SetValue(display, property.m_mode);
        return false;
    }

    ////////////////////////////////////////////////////////////////////////

} // namespace FoundationLocalisation
