#pragma once
#include "Backend/Segmentation/LabelMetaData.hpp"
#include <QAbstractListModel>
#include <QVariantList>

// GUI-thread model. Render commands receive value copies, never model references.
class LabelListModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int activeLabel READ activeLabel NOTIFY activeLabelChanged)
    Q_PROPERTY(QVariantList palette READ palette CONSTANT)
public:
    enum Role
    {
        CaptionRole = Qt::UserRole + 1,
        ColorRole,
        LabelRole,
        VisibilityRole
    };
    explicit LabelListModel(QObject* parent = nullptr) :
        QAbstractListModel(parent)
    {
    }
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int activeLabel() const
    {
        return activeLabel_;
    }
    QVariantList palette() const;
    const std::vector<LabelMetaData>& items() const
    {
        return items_;
    }
    Q_INVOKABLE int add(QColor color);
    Q_INVOKABLE void select(int label);
    Q_INVOKABLE void rename(int label, QString name);
    Q_INVOKABLE void setVisible(int label, bool visible);
    Q_INVOKABLE void remove(int label);
    Q_INVOKABLE bool containsColor(QColor color) const;
    void clear();
    void replace(std::vector<LabelMetaData> items);
signals:
    void activeLabelChanged();
    void labelsChanged();
    void labelRemoved(int label);

private:
    int rowFor(int label) const;
    int activeLabel_ = 0;
    std::vector<LabelMetaData> items_;
};
