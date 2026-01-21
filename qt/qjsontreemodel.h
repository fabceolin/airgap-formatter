#ifndef QJSONTREEMODEL_H
#define QJSONTREEMODEL_H

#include <QAbstractItemModel>
#include <QJsonDocument>
#include "qjsontreeitem.h"

class QJsonTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles {
        KeyRole = Qt::UserRole + 1,
        ValueRole,
        ValueTypeRole,
        JsonPathRole,
        ChildCountRole,
        IsExpandableRole
    };
    Q_ENUM(Roles)

    explicit QJsonTreeModel(QObject* parent = nullptr);
    ~QJsonTreeModel() override;

    // QAbstractItemModel interface
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // JSON loading
    Q_INVOKABLE bool loadJson(const QString& jsonString);
    Q_INVOKABLE void clear();

    // Serialization for copy functionality
    Q_INVOKABLE QString serializeNode(const QModelIndex& index) const;
    Q_INVOKABLE QString getJsonPath(const QModelIndex& index) const;

signals:
    void loadError(const QString& error);

private:
    QJsonTreeItem* m_rootItem;
};

#endif // QJSONTREEMODEL_H
