#ifndef QJSONTREEITEM_H
#define QJSONTREEITEM_H

#include <QString>
#include <QVariant>
#include <QVector>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>

class QJsonTreeItem
{
public:
    enum class Type { Object, Array, String, Number, Boolean, Null };

    explicit QJsonTreeItem(QJsonTreeItem* parent = nullptr);
    ~QJsonTreeItem();

    void appendChild(QJsonTreeItem* child);
    QJsonTreeItem* child(int row) const;
    int childCount() const;
    int row() const;
    QJsonTreeItem* parentItem() const;

    // Data accessors
    QString key() const;
    void setKey(const QString& key);

    QVariant value() const;
    void setValue(const QVariant& value);

    Type type() const;
    void setType(Type type);

    QString typeName() const;
    QString jsonPath() const;
    bool isExpandable() const;

    // Serialization for copy functionality
    QString toJsonString(int indentLevel = 0) const;

    // Factory method to build tree from JSON
    static QJsonTreeItem* load(const QJsonValue& value, QJsonTreeItem* parent = nullptr);

private:
    QVector<QJsonTreeItem*> m_children;
    QJsonTreeItem* m_parent;
    QString m_key;
    QVariant m_value;
    Type m_type;
};

#endif // QJSONTREEITEM_H
