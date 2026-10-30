#pragma once

#include <QAbstractListModel>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

struct RecordingItem {
    QString id;
    int sourceId = 0;
    QString sourceName;
    QString path;
    QString startedAt;
    QString endedAt;
    qint64 durationMs = 0;
    int width = 0;
    int height = 0;
    double fps = 0.0;
    QString status;
};

class RecordingModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        SourceIdRole,
        SourceNameRole,
        PathRole,
        StartedAtRole,
        EndedAtRole,
        DurationMsRole,
        WidthRole,
        HeightRole,
        FpsRole,
        StatusRole,
    };
    Q_ENUM(Role)

    explicit RecordingModel(const QSqlDatabase &database,
                            QString recordingDirectory,
                            QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    QString lastError() const;

    Q_INVOKABLE void reload();
    Q_INVOKABLE bool reindex();
    Q_INVOKABLE QVariantMap itemAt(int row) const;

signals:
    void countChanged();
    void lastErrorChanged();

private:
    bool ensureDatabaseOpen();
    bool loadFromDatabase();
    bool insertIndexRow(const QString &path);
    void setLastError(const QString &message);
    void clearLastError();

    QSqlDatabase m_database;
    QString m_recordingDirectory;
    QVector<RecordingItem> m_items;
    QString m_lastError;
};
