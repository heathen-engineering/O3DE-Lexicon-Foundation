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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

class QLineEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;

namespace FoundationLocalisation
{
    ///<summary>
    /// Modal key picker overlay opened by LexiconPropertyWidget when in Localised mode.
    ///
    /// Layout:
    ///   [ Filter line edit                         ]
    ///   [ QTreeWidget — key hierarchy (dot tokens) ]
    ///   [           Cancel ]  [ OK ]
    ///
    /// The tree is built by tokenising every known dot-path key from
    /// LexiconEditorRequestBus::GetKnownKeys() on '.'. Each item stores its
    /// full dot-path in Qt::UserRole so partial branch paths can be selected
    /// if they correspond to a real key.
    ///
    /// The filter narrows visible items in real time (case-insensitive substring
    /// match on the full path). Parents of matching items are kept visible and
    /// auto-expanded.
    ///
    /// Accepted with a selection → GetSelectedKey() returns the chosen dot-path.
    /// Rejected or no selection  → GetSelectedKey() returns an empty string.
    ///</summary>
    class LexiconKeyPickerDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit LexiconKeyPickerDialog(QWidget* parent = nullptr);

        ///<summary>Pre-selects the given key in the tree on open.</summary>
        void SetCurrentKey(const AZStd::string& key);

        ///<summary>Returns the chosen dot-path key, or empty if cancelled.</summary>
        AZStd::string GetSelectedKey() const;

    private slots:
        void OnFilterChanged(const QString& text);
        void OnCurrentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
        void OnItemDoubleClicked(QTreeWidgetItem* item, int column);

    private:
        void PopulateTree(const AZStd::vector<AZStd::string>& keys);

        ///<summary>
        /// Recursively shows/hides items based on filter.
        /// Returns true if this item or any descendant should be visible.
        ///</summary>
        bool UpdateItemVisibility(QTreeWidgetItem* item, const QString& filter);

        QLineEdit*   m_filterEdit = nullptr;
        QTreeWidget* m_tree       = nullptr;
        QPushButton* m_okButton   = nullptr;
    };

} // namespace FoundationLocalisation
