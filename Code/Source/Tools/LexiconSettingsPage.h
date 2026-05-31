#pragma once

#include <EditorExtensions/ISettingsPage.h>

namespace FoundationLocalisation
{
    class LexiconSettingsPage : public EditorExtensions::ISettingsPage
    {
    public:
        AZStd::string GetPath() const override;
        QWidget* CreateWidget(QWidget* parent) override;
    };

} // namespace FoundationLocalisation
