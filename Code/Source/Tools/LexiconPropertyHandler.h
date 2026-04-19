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

#include "LexiconPropertyWidget.h"

#include <FoundationLocalisation/LexiconText.h>
#include <FoundationLocalisation/LexiconSound.h>
#include <FoundationLocalisation/LexiconAsset.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

namespace FoundationLocalisation
{
    ///<summary>
    /// Property handler for Heathen::LexiconText.
    /// Displays a mode toggle and a key/value string field.
    /// Registered as the default handler for the LexiconText type.
    ///</summary>
    class LexiconTextPropertyHandler
        : public AzToolsFramework::PropertyHandler<Heathen::LexiconText, LexiconPropertyWidget>
    {
    public:
        AZ_CLASS_ALLOCATOR(LexiconTextPropertyHandler, AZ::SystemAllocator, 0);

        AZ::u32  GetHandlerName() const override { return 0; }
        bool     AutoDelete()     const override { return false; }

        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(
            LexiconPropertyWidget*, AZ::u32,
            AzToolsFramework::PropertyAttributeReader*, const char*) override {}

        void WriteGUIValuesIntoProperty(
            size_t index, LexiconPropertyWidget* widget,
            Heathen::LexiconText& property,
            AzToolsFramework::InstanceDataNode* node) override;

        bool ReadValuesIntoGUI(
            size_t index, LexiconPropertyWidget* widget,
            const Heathen::LexiconText& property,
            AzToolsFramework::InstanceDataNode* node) override;
    };

    ///<summary>
    /// Property handler for Heathen::LexiconSound.
    /// In Literal/Invariant mode the UUID from m_literalAssetId is shown as a
    /// string. A proper asset-picker integration can replace this in a later pass.
    ///</summary>
    class LexiconSoundPropertyHandler
        : public AzToolsFramework::PropertyHandler<Heathen::LexiconSound, LexiconPropertyWidget>
    {
    public:
        AZ_CLASS_ALLOCATOR(LexiconSoundPropertyHandler, AZ::SystemAllocator, 0);

        AZ::u32  GetHandlerName() const override { return 0; }
        bool     AutoDelete()     const override { return false; }

        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(
            LexiconPropertyWidget*, AZ::u32,
            AzToolsFramework::PropertyAttributeReader*, const char*) override {}

        void WriteGUIValuesIntoProperty(
            size_t index, LexiconPropertyWidget* widget,
            Heathen::LexiconSound& property,
            AzToolsFramework::InstanceDataNode* node) override;

        bool ReadValuesIntoGUI(
            size_t index, LexiconPropertyWidget* widget,
            const Heathen::LexiconSound& property,
            AzToolsFramework::InstanceDataNode* node) override;
    };

    ///<summary>
    /// Property handler for Heathen::LexiconAsset.
    /// Identical behaviour to LexiconSoundPropertyHandler — shows UUID string
    /// in Literal/Invariant mode.
    ///</summary>
    class LexiconAssetPropertyHandler
        : public AzToolsFramework::PropertyHandler<Heathen::LexiconAsset, LexiconPropertyWidget>
    {
    public:
        AZ_CLASS_ALLOCATOR(LexiconAssetPropertyHandler, AZ::SystemAllocator, 0);

        AZ::u32  GetHandlerName() const override { return 0; }
        bool     AutoDelete()     const override { return false; }

        QWidget* CreateGUI(QWidget* parent) override;

        void ConsumeAttribute(
            LexiconPropertyWidget*, AZ::u32,
            AzToolsFramework::PropertyAttributeReader*, const char*) override {}

        void WriteGUIValuesIntoProperty(
            size_t index, LexiconPropertyWidget* widget,
            Heathen::LexiconAsset& property,
            AzToolsFramework::InstanceDataNode* node) override;

        bool ReadValuesIntoGUI(
            size_t index, LexiconPropertyWidget* widget,
            const Heathen::LexiconAsset& property,
            AzToolsFramework::InstanceDataNode* node) override;
    };

} // namespace FoundationLocalisation
