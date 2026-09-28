#include "clipmodel.h"
#include "clipstorage.h"

#include <QDate>
#include <QDateTime>
#include <QFileInfo>
#include <QHash>
#include <QLocale>
#include <QTime>

static bool asphaltcam_incident_times(const QString &path, QDateTime *start, QDateTime *end)
{
    const QString base = QFileInfo(path).fileName();
    const QDate date = QDate::fromString(base.left(8), QStringLiteral("yyyyMMdd"));
    const QTime t0 = QTime::fromString(base.mid(9, 6), QStringLiteral("HHmmss"));
    const QTime t1 = QTime::fromString(base.mid(16, 6), QStringLiteral("HHmmss"));
    if (!date.isValid() || !t0.isValid() || !t1.isValid())
        return false;
    *start = QDateTime(date, t0);
    *end = QDateTime(date, t1);
    return true;
}

ClipModel::ClipModel(ClipStorage *storage, QObject *parent)
    : QAbstractListModel(parent)
    , m_storage(storage)
{
    connect(m_storage, SIGNAL(clipsChanged()), this, SLOT(refresh()));
    connect(m_storage, SIGNAL(fileRemoved(QString)), this, SLOT(onFileRemoved(QString)));
    refresh();
}

int ClipModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

QVariant ClipModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();
    const QString path = m_items.at(index.row());
    switch (role) {
    case PathRole:
        return path;
    case TitleRole: {
        QDateTime start;
        QDateTime end;
        if (!asphaltcam_incident_times(path, &start, &end))
            return QFileInfo(path).fileName();
        return QLocale().toString(start.date(), QLocale::LongFormat);
    }
    case SubtitleRole: {
        QDateTime start;
        QDateTime end;
        if (!asphaltcam_incident_times(path, &start, &end))
            return QString();
        return start.time().toString(QStringLiteral("HH:mm:ss")) + QStringLiteral(" \u2013 ")
                + end.time().toString(QStringLiteral("HH:mm:ss"));
    }
    case PartsRole:
        return m_storage->incidentParts(path);
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ClipModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[PathRole] = "path";
    roles[TitleRole] = "title";
    roles[SubtitleRole] = "subtitle";
    roles[PartsRole] = "parts";
    return roles;
}

void ClipModel::refresh()
{
    beginResetModel();
    m_items = m_storage->incidentFiles();
    endResetModel();
    emit countChanged();
}

void ClipModel::onFileRemoved(const QString &path)
{
    const int i = m_items.indexOf(path);
    if (i < 0)
        return;
    beginRemoveRows(QModelIndex(), i, i);
    m_items.removeAt(i);
    endRemoveRows();
    emit countChanged();
}
