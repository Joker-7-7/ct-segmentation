#include "LabelListModel.hpp"
int LabelListModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(items_.size());
}

QVariant LabelListModel::data(const QModelIndex& index, int role) const
{
    if (index.isValid() == false || index.row() < 0 || index.row() >= rowCount())
    {
        return {};
    }
    const auto& item = items_[index.row()];
    switch (role)
    {
        case CaptionRole:
            return item.name;
        case ColorRole:
            return item.color;
        case LabelRole:
            return item.label;
        case VisibilityRole:
            return item.isVisible;
        default:
            return {};
    }
}

QHash<int, QByteArray> LabelListModel::roleNames() const
{
    return { { CaptionRole, "caption" },
             { ColorRole, "labelColor" },
             { LabelRole, "colorIndex" },
             { VisibilityRole, "labelVisible" } };
}

int LabelListModel::rowFor(int label) const
{
    for (int i = 0; i < rowCount(); ++i)
    {
        if (items_[i].label == label)
        {
            return i;
        }
    }
    return -1;
}

QVariantList LabelListModel::palette() const
{
    QVariantList result;
    for (const auto& c : LabelMetaData::Palette())
    {
        if (c.isValid() && result.contains(QVariant(c)) == false)
        {
            result.push_back(c);
        }
    }
    return result;
}

bool LabelListModel::containsColor(QColor color) const
{
    return rowFor(LabelMetaData::EncodeColor(color)) >= 0;
}

int LabelListModel::add(QColor color)
{
    const auto id = LabelMetaData::EncodeColor(color);
    if (id == 0)
    {
        return 0;
    }
    if (rowFor(id) < 0)
    {
        const auto row = rowCount();
        items_.reserve(items_.size() + 1);
        beginInsertRows({}, row, row);
        items_.push_back({ id, QString("Label #%1").arg(id), color, true });
        endInsertRows();
        emit labelsChanged();
    }
    select(id);
    return id;
}

void LabelListModel::select(int label)
{
    if ((label != 0 && rowFor(label) < 0) || label == activeLabel_)
    {
        return;
    }
    activeLabel_ = label;
    emit activeLabelChanged();
}

void LabelListModel::rename(int label, QString name)
{
    const auto row = rowFor(label);
    name = name.trimmed();
    if (row < 0 || name.isEmpty() || items_[row].name == name)
    {
        return;
    }
    items_[row].name = std::move(name);
    emit dataChanged(index(row), index(row), { CaptionRole });
    emit labelsChanged();
}

void LabelListModel::setVisible(int label, bool visible)
{
    const auto row = rowFor(label);
    if (row < 0 || items_[row].isVisible == visible)
    {
        return;
    }
    items_[row].isVisible = visible;
    emit dataChanged(index(row), index(row), { VisibilityRole });
    emit labelsChanged();
}

void LabelListModel::remove(int label)
{
    const auto row = rowFor(label);
    if (row < 0)
    {
        return;
    }
    beginRemoveRows({}, row, row);
    items_.erase(items_.begin() + row);
    endRemoveRows();
    emit labelRemoved(label);
    emit labelsChanged();
    if (activeLabel_ == label)
    {
        select(items_.empty() ? 0 : items_.front().label);
    }
}

void LabelListModel::clear()
{
    beginResetModel();
    items_.clear();
    endResetModel();
    select(0);
    emit labelsChanged();
}

void LabelListModel::replace(std::vector<LabelMetaData> items)
{
    beginResetModel();
    items_ = std::move(items);
    activeLabel_ = items_.empty() ? 0 : items_.front().label;
    endResetModel();
    emit labelsChanged();
    emit activeLabelChanged();
}
