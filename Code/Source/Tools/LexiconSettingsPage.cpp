#include "LexiconSettingsPage.h"
#include "LexiconToolWindow.h"

namespace FoundationLocalisation
{
    AZStd::string LexiconSettingsPage::GetPath() const
    {
        return "Localisation Lexicon";
    }

    QWidget* LexiconSettingsPage::CreateWidget(QWidget* parent)
    {
        return new LexiconToolWindow(parent);
    }

} // namespace FoundationLocalisation
