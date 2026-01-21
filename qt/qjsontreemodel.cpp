#include "qjsontreemodel.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

QJsonTreeModel::QJsonTreeModel(QObject* parent)
    : QAbstractItemModel(parent)
    , m_rootItem(nullptr)
{
}

QJsonTreeModel::~QJsonTreeModel()
{
    delete m_rootItem;
}

QModelIndex QJsonTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    QJsonTreeItem* parentItem;
    if (!parent.isValid()) {
        parentItem = m_rootItem;
    } else {
        parentItem = static_cast<QJsonTreeItem*>(parent.internalPointer());
    }

    if (!parentItem)
        return QModelIndex();

    QJsonTreeItem* childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);

    return QModelIndex();
}

QModelIndex QJsonTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid())
        return QModelIndex();

    QJsonTreeItem* childItem = static_cast<QJsonTreeItem*>(index.internalPointer());
    if (!childItem)
        return QModelIndex();

    QJsonTreeItem* parentItem = childItem->parentItem();

    if (!parentItem || parentItem == m_rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int QJsonTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.column() > 0)
        return 0;

    QJsonTreeItem* parentItem;
    if (!parent.isValid()) {
        parentItem = m_rootItem;
    } else {
        parentItem = static_cast<QJsonTreeItem*>(parent.internalPointer());
    }

    if (!parentItem)
        return 0;

    return parentItem->childCount();
}

int QJsonTreeModel::columnCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QVariant QJsonTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QJsonTreeItem* item = static_cast<QJsonTreeItem*>(index.internalPointer());
    if (!item)
        return QVariant();

    switch (role) {
        case KeyRole:
            return item->key();
        case ValueRole:
            return item->value();
        case ValueTypeRole:
            return item->typeName();
        case JsonPathRole:
            return item->jsonPath();
        case ChildCountRole:
            return item->childCount();
        case IsExpandableRole:
            return item->isExpandable();
        case IsLastChildRole: {
            QJsonTreeItem* parent = item->parentItem();
            if (!parent)
                return false;
            return (parent->childCount() - 1) == index.row();
        }
        case ParentValueTypeRole: {
            QJsonTreeItem* parent = item->parentItem();
            if (!parent)
                return QString();
            return parent->typeName();
        }
        case Qt::DisplayRole:
            // For display, combine key and value
            if (item->key().isEmpty()) {
                return item->value();
            }
            return item->key() + ": " + item->value().toString();
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> QJsonTreeModel::roleNames() const
{
    return {
        {KeyRole, "key"},
        {ValueRole, "value"},
        {ValueTypeRole, "valueType"},
        {JsonPathRole, "jsonPath"},
        {ChildCountRole, "childCount"},
        {IsExpandableRole, "isExpandable"},
        {IsLastChildRole, "isLastChild"},
        {ParentValueTypeRole, "parentValueType"}
    };
}

bool QJsonTreeModel::loadJson(const QString& jsonString)
{
    beginResetModel();

    delete m_rootItem;
    m_rootItem = nullptr;

    if (jsonString.trimmed().isEmpty()) {
        endResetModel();
        return true;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        emit loadError(error.errorString());
        endResetModel();
        return false;
    }

    // Create a virtual root to hold the actual JSON root
    m_rootItem = new QJsonTreeItem();
    m_rootItem->setType(QJsonTreeItem::Type::Object);

    QJsonTreeItem* jsonRoot;
    if (doc.isObject()) {
        jsonRoot = QJsonTreeItem::load(doc.object(), m_rootItem);
    } else if (doc.isArray()) {
        jsonRoot = QJsonTreeItem::load(doc.array(), m_rootItem);
    } else {
        // Handle root-level primitives (less common but valid)
        jsonRoot = QJsonTreeItem::load(QJsonValue(), m_rootItem);
    }

    m_rootItem->appendChild(jsonRoot);

    endResetModel();
    return true;
}

void QJsonTreeModel::clear()
{
    beginResetModel();
    delete m_rootItem;
    m_rootItem = nullptr;
    endResetModel();
}

QString QJsonTreeModel::serializeNode(const QModelIndex& index) const
{
    if (!index.isValid())
        return QString();

    QJsonTreeItem* item = static_cast<QJsonTreeItem*>(index.internalPointer());
    if (!item)
        return QString();

    return item->toJsonString();
}

QString QJsonTreeModel::getJsonPath(const QModelIndex& index) const
{
    if (!index.isValid())
        return QString();

    QJsonTreeItem* item = static_cast<QJsonTreeItem*>(index.internalPointer());
    if (!item)
        return QString();

    return item->jsonPath();
}

int QJsonTreeModel::totalNodeCount() const
{
    if (!m_rootItem)
        return 0;

    return countNodes(m_rootItem);
}

int QJsonTreeModel::countNodes(QJsonTreeItem* item) const
{
    if (!item)
        return 0;

    int count = 1;  // Count this node
    for (int i = 0; i < item->childCount(); ++i) {
        count += countNodes(item->child(i));
    }
    return count;
}
