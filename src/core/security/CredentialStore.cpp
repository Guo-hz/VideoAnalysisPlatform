#include "CredentialStore.h"

#include <QUrl>
#include <QUuid>
#include <QStringList>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincred.h>
#endif

namespace {
void setError(QString *error, const QString &message)
{
    if (error) {
        *error = message;
    }
}
}

bool CredentialStore::write(const QString &reference, const Secret &secret, QString *error)
{
    if (reference.trimmed().isEmpty() || secret.endpoint.trimmed().isEmpty()) {
        setError(error, QStringLiteral("凭据引用或连接地址为空。"));
        return false;
    }

#ifdef Q_OS_WIN
    const QString payload = secret.endpoint + QChar(0x1f) + secret.username + QChar(0x1f)
                            + secret.password;
    const QByteArray payloadBytes(reinterpret_cast<const char *>(payload.utf16()),
                                  payload.size() * static_cast<int>(sizeof(char16_t)));
    CREDENTIALW credential{};
    const std::wstring target = reference.toStdWString();
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = const_cast<LPWSTR>(target.c_str());
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
    credential.CredentialBlobSize = static_cast<DWORD>(payloadBytes.size());
    credential.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char *>(payloadBytes.constData()));
    if (!CredWriteW(&credential, 0)) {
        setError(error, QStringLiteral("写入 Windows Credential Manager 失败（错误码 %1）。")
                           .arg(static_cast<qulonglong>(GetLastError())));
        return false;
    }
    return true;
#else
    Q_UNUSED(reference)
    Q_UNUSED(secret)
    setError(error, QStringLiteral("当前平台不支持 Windows Credential Manager。"));
    return false;
#endif
}

bool CredentialStore::read(const QString &reference, Secret *secret, QString *error)
{
    if (!secret || reference.trimmed().isEmpty()) {
        setError(error, QStringLiteral("凭据引用无效。"));
        return false;
    }

#ifdef Q_OS_WIN
    PCREDENTIALW credential = nullptr;
    const std::wstring target = reference.toStdWString();
    if (!CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &credential)) {
        setError(error, QStringLiteral("读取 Windows Credential Manager 失败（错误码 %1）。")
                           .arg(static_cast<qulonglong>(GetLastError())));
        return false;
    }
    const QString payload = QString::fromUtf16(
        reinterpret_cast<const ushort *>(credential->CredentialBlob),
        static_cast<int>(credential->CredentialBlobSize / sizeof(char16_t)));
    const QStringList fields = payload.split(QChar(0x1f));
    CredFree(credential);
    if (fields.size() < 3) {
        setError(error, QStringLiteral("凭据内容格式无效。"));
        return false;
    }
    secret->endpoint = fields.at(0);
    secret->username = fields.at(1);
    secret->password = fields.mid(2).join(QChar(0x1f));
    return true;
#else
    Q_UNUSED(reference)
    Q_UNUSED(secret)
    setError(error, QStringLiteral("当前平台不支持 Windows Credential Manager。"));
    return false;
#endif
}

bool CredentialStore::remove(const QString &reference, QString *error)
{
    if (reference.trimmed().isEmpty()) {
        return true;
    }
#ifdef Q_OS_WIN
    const std::wstring target = reference.toStdWString();
    if (!CredDeleteW(target.c_str(), CRED_TYPE_GENERIC, 0)) {
        const DWORD code = GetLastError();
        if (code == ERROR_NOT_FOUND) {
            return true;
        }
        setError(error, QStringLiteral("删除 Windows Credential Manager 凭据失败（错误码 %1）。")
                           .arg(static_cast<qulonglong>(code)));
        return false;
    }
    return true;
#else
    Q_UNUSED(reference)
    setError(error, QStringLiteral("当前平台不支持 Windows Credential Manager。"));
    return false;
#endif
}

QString CredentialStore::newReference()
{
    return QStringLiteral("VideoAnalysisPlatform/source/%1")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

QString CredentialStore::sanitizeEndpoint(const QString &endpoint)
{
    QUrl url(endpoint);
    if (!url.isValid()) {
        return QStringLiteral("[redacted]");
    }
    url.setUserName(QString());
    url.setPassword(QString());
    if (!url.query(QUrl::FullyEncoded).isEmpty()) {
        url.setQuery(QStringLiteral("[redacted]"));
    }
    return url.toString(QUrl::FullyEncoded);
}

bool CredentialStore::isNetworkType(const QString &type)
{
    return type.compare(QStringLiteral("RTSP"), Qt::CaseInsensitive) == 0
           || type.compare(QStringLiteral("HTTP"), Qt::CaseInsensitive) == 0;
}
