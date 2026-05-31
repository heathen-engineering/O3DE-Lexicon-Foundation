#include "LexiconHintTypeDelegate.h"
#include "LexiconTableModel.h"

#include <QComboBox>

namespace FoundationLocalisation
{
    LexiconHintTypeDelegate::LexiconHintTypeDelegate(QObject* parent)
        : QStyledItemDelegate(parent)
    {
    }

    QWidget* LexiconHintTypeDelegate::createEditor(QWidget* parent,
                                                    const QStyleOptionViewItem& /*option*/,
                                                    const QModelIndex& /*index*/) const
    {
        auto* combo = new QComboBox(parent);
        combo->addItems(LexiconTableModel::HintTypeLabels());
        return combo;
    }

    void LexiconHintTypeDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
    {
        auto* combo = qobject_cast<QComboBox*>(editor);
        if (!combo) return;

        const int hintInt = index.data(Qt::EditRole).toInt();
        const QString label = LexiconTableModel::HintTypeLabel(
            static_cast<LexiconHintType>(hintInt));
        combo->setCurrentText(label);
    }

    void LexiconHintTypeDelegate::setModelData(QWidget* editor,
                                                QAbstractItemModel* model,
                                                const QModelIndex& index) const
    {
        auto* combo = qobject_cast<QComboBox*>(editor);
        if (!combo) return;

        const int idx  = combo->currentIndex();
        const auto hint = LexiconTableModel::HintTypeFromIndex(idx);
        model->setData(index, static_cast<int>(hint), Qt::EditRole);
    }

    void LexiconHintTypeDelegate::updateEditorGeometry(QWidget* editor,
                                                        const QStyleOptionViewItem& option,
                                                        const QModelIndex& /*index*/) const
    {
        editor->setGeometry(option.rect);
    }

} // namespace FoundationLocalisation

#include <moc_LexiconHintTypeDelegate.cpp>
