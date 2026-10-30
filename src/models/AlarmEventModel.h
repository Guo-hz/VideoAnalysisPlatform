#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

#include "core/alarm/AlarmEvent.h"
#include "core/storage/AlarmEventRepository.h"

class AlarmEventModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString eventTypeFilter READ eventTypeFilter WRITE setEventTypeFilter NOTIFY filtersChanged)
    Q_PROPERTY(QString sourceFilter READ sourceFilter WRITE setSourceFilter NOTIFY filtersChanged)
    Q_PROPERTY(int confirmedFilter READ confirmedFilter WRITE setConfirmedFilter NOTIFY filtersChanged)
    Q_PROPERTY(QString mediaDirectory READ mediaDirectory WRITE setMediaDirectory NOTIFY mediaDirectoryChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        SourceIdRole,
        VideoSourceIdRole,
        LegacySourceKeyRole,
        SourceNameRole,
        EventTypeRole,
        LevelRole,
        MessageRole,
        SnapshotPathRole,
        VideoClipPathRole,
        ConfirmedRole,
        TimestampRole
    };
    Q_ENUM(Role)

    explicit AlarmEventModel(AlarmEventRepository *repository, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    QString lastError() const;
    QString eventTypeFilter() const;
    QString sourceFilter() const;
    int confirmedFilter() const;
    void setEventTypeFilter(const QString &value);
    void setSourceFilter(const QString &value);
    void setConfirmedFilter(int value);
    QString mediaDirectory() const;
    void setMediaDirectory(const QString &value);

    Q_INVOKABLE int addTestAlarm();
    Q_INVOKABLE bool confirmAlarm(int row);
    Q_INVOKABLE bool removeAlarm(int row);
    Q_INVOKABLE bool removeAlarmWithMedia(int row, bool deleteMedia);
    Q_INVOKABLE void reload();

public slots:
    void addAlarm(const AlarmEvent &event);

signals:
    void countChanged();
    void lastErrorChanged();
    void filtersChanged();
    void mediaDirectoryChanged();

private:
    bool isValidRow(int row) const;
    void loadFromRepository();
    void clearLastError();
    void setLastError(const QString &message);
    void applyFilters();
    bool matchesFilters(const AlarmEvent &event) const;

    AlarmEventRepository *m_repository = nullptr;
    QVector<AlarmEvent> m_events;
    QVector<AlarmEvent> m_allEvents;
    int m_nextTestAlarmIndex = 1;
    QString m_lastError;
    QString m_eventTypeFilter;
    QString m_sourceFilter;
    int m_confirmedFilter = -1;
    QString m_mediaDirectory;
};
