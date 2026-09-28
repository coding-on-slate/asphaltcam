#ifndef CLIPMODEL_H
#define CLIPMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QStringList>

class ClipStorage;

class ClipModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        TitleRole,
        SubtitleRole,
        PartsRole
    };

    explicit ClipModel(ClipStorage *storage, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;

public slots:
    void refresh();
    void onFileRemoved(const QString &path);

signals:
    void countChanged();

private:
    ClipStorage *m_storage;
    QStringList m_items;
};

#endif
