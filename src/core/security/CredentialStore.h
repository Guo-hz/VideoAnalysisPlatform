#pragma once

#include <QString>

class CredentialStore
{
public:
    struct Secret {
        QString endpoint;
        QString username;
        QString password;
    };

    static bool write(const QString &reference, const Secret &secret, QString *error = nullptr);
    static bool read(const QString &reference, Secret *secret, QString *error = nullptr);
    static bool remove(const QString &reference, QString *error = nullptr);

    static QString newReference();
    static QString sanitizeEndpoint(const QString &endpoint);
    static bool isNetworkType(const QString &type);
};
