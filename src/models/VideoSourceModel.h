#pragma once

#include <QAbstractListModel>
#include <QVariantMap>
#include <QString>
#include <QVector>

#include "core/storage/VideoSourceRepository.h"
#include "core/video/VideoSource.h"

class VideoSourceModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        NameRole,
        TypeRole,
        UrlRole,
        EnabledRole,
        StatusRole,
        LastErrorRole,
        SourceEnabledRole,
        CredentialRefRole,
        ReconnectMaxAttemptsRole,
        ReconnectIntervalMsRole,
    };
    Q_ENUM(Role)

    explicit VideoSourceModel(VideoSourceRepository *repository, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    QString lastError() const;

    Q_INVOKABLE QVariantMap sourceAt(int row) const;
    Q_INVOKABLE int addSource(const QString &name,
                              const QString &type,
                              const QString &endpoint,
                              const QString &username = QString(),
                              const QString &password = QString());
    Q_INVOKABLE bool updateSource(int row,
                                  const QString &name,
                                  const QString &type,
                                  const QString &endpoint,
                                  const QString &username = QString(),
                                  const QString &password = QString());
    Q_INVOKABLE bool setSourceEnabled(int row, bool enabled);
    Q_INVOKABLE bool removeSource(int row);
    Q_INVOKABLE bool renameSource(int row, const QString &name);
    Q_INVOKABLE bool testSource(int row);

    // Compatibility helpers for the initial placeholder UI.
    Q_INVOKABLE int addLocalPlaceholderSource();
    Q_INVOKABLE int addRtspPlaceholderSource();

signals:
    void countChanged();
    void lastErrorChanged();
    void sourceChanged(int sourceId);
    void sourceTestFinished(int sourceId, bool success, const QString &message);

private:
    bool isValidRow(int row) const;
    bool buildSource(VideoSource *source,
                     const QString &name,
                     const QString &type,
                     const QString &endpoint,
                     const QString &username,
                     const QString &password,
                     const VideoSource *existing = nullptr);
    bool persistCredential(VideoSource *source,
                           const QString &endpoint,
                           const QString &username,
                           const QString &password,
                           const VideoSource *existing);
    void appendSource(const VideoSource &source);
    void loadFromRepository();
    void clearLastError();
    void setLastError(const QString &message);

    VideoSourceRepository *m_repository = nullptr;
    QVector<VideoSource> m_sources;
    int m_nextPlaceholderIndex = 1;
    QString m_lastError;
};
