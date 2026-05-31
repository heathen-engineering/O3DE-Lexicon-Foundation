#pragma once

#include <QStyledItemDelegate>

namespace FoundationLocalisation
{
    /// Item delegate for the Type column in the Lexicon Workbench table.
    /// Shows a QComboBox with all LexiconHintType options (excluding None).
    class LexiconHintTypeDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        explicit LexiconHintTypeDelegate(QObject* parent = nullptr);

        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const override;

        void setEditorData(QWidget* editor, const QModelIndex& index) const override;

        void setModelData(QWidget* editor, QAbstractItemModel* model,
                          const QModelIndex& index) const override;

        void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const override;
    };

} // namespace FoundationLocalisation
