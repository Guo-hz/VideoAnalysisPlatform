#include "RoiRuleRepository.h"

#include <algorithm>

#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
constexpr int kDefaultCooldownSeconds = 10;

QString rectToJson(const QRectF &rect)
{
    const QRectF normalized = rect.normalized();
    QJsonObject object;
    object.insert(QStringLiteral("x"), normalized.x());
    object.insert(QStringLiteral("y"), normalized.y());
    object.insert(QStringLiteral("width"), normalized.width());
    object.insert(QStringLiteral("height"), normalized.height());
    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}

QRectF rectFromJson(const QString &json, bool *ok)
{
    if (ok) {
        *ok = false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8());
    if (!document.isObject()) {
        return {};
    }

    const QJsonObject object = document.object();
    const QRectF rect(object.value(QStringLiteral("x")).toDouble(),
                      object.value(QStringLiteral("y")).toDouble(),
                      object.value(QStringLiteral("width")).toDouble(),
                      object.value(QStringLiteral("height")).toDouble());
    if (ok) {
        *ok = rect.isValid();
    }

    return rect.normalized();
}

RoiRule readRule(const QSqlQuery &query)
{
    RoiRule rule;
    rule.id = query.value(0).toInt();
    rule.sourceId = query.value(1).toInt();
    rule.ruleName = query.value(2).toString();
    rule.enabled = query.value(3).toInt() != 0;

    bool rectOk = false;
    rule.rect = rectFromJson(query.value(4).toString(), &rectOk);
    if (!rectOk) {
        rule.rect = QRectF();
    }

    bool cooldownOk = false;
    const int cooldownSeconds = query.value(5).toInt(&cooldownOk);
    rule.cooldownSeconds = cooldownOk ? std::max(0, cooldownSeconds) : kDefaultCooldownSeconds;
    rule.createdAt = query.value(6).toString();
    rule.updatedAt = query.value(7).toString();
    return rule;
}
}

RoiRuleRepository::RoiRuleRepository(const QSqlDatabase &database)
    : m_database(database)
{
}

QVector<RoiRule> RoiRuleRepository::loadAll()
{
    QVector<RoiRule> rules;
    if (!ensureDatabaseOpen()) {
        return rules;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT id, source_id, rule_name, enabled, points_json,
               cooldown_seconds, created_at, updated_at
        FROM roi_rules
        ORDER BY source_id ASC, id ASC
    )"));

    return loadByQuery(query, QStringLiteral("读取 ROI 规则失败"));
}

QVector<RoiRule> RoiRuleRepository::loadBySource(int sourceId)
{
    QVector<RoiRule> rules;
    if (!ensureDatabaseOpen()) {
        return rules;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        SELECT id, source_id, rule_name, enabled, points_json,
               cooldown_seconds, created_at, updated_at
        FROM roi_rules
        WHERE source_id = :source_id
        ORDER BY id ASC
    )"));
    query.bindValue(QStringLiteral(":source_id"), sourceId);

    return loadByQuery(query, QStringLiteral("读取视频源 ROI 规则失败"));
}

bool RoiRuleRepository::add(RoiRule *rule)
{
    if (!rule) {
        setLastError(QStringLiteral("新增 ROI 规则失败"), QStringLiteral("rule 为空"));
        return false;
    }
    if (!ensureDatabaseOpen()) {
        return false;
    }

    const QRectF rect = rule->rect.normalized();
    if (!rect.isValid() || rect.isEmpty()) {
        setLastError(QStringLiteral("新增 ROI 规则失败"), QStringLiteral("ROI 矩形无效"));
        return false;
    }

    const QString now = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        INSERT INTO roi_rules (
            source_id, rule_name, enabled, roi_type, points_json,
            sensitivity, min_target_area, cooldown_seconds, created_at, updated_at
        )
        VALUES (
            :source_id, :rule_name, :enabled, :roi_type, :points_json,
            :sensitivity, :min_target_area, :cooldown_seconds, :created_at, :updated_at
        )
    )"));
    query.bindValue(QStringLiteral(":source_id"), rule->sourceId);
    query.bindValue(QStringLiteral(":rule_name"), rule->ruleName.trimmed().isEmpty() ? QStringLiteral("ROI") : rule->ruleName.trimmed());
    query.bindValue(QStringLiteral(":enabled"), rule->enabled ? 1 : 0);
    query.bindValue(QStringLiteral(":roi_type"), QStringLiteral("rect"));
    query.bindValue(QStringLiteral(":points_json"), rectToJson(rect));
    query.bindValue(QStringLiteral(":sensitivity"), 0.5);
    query.bindValue(QStringLiteral(":min_target_area"), 0);
    query.bindValue(QStringLiteral(":cooldown_seconds"), std::max(0, rule->cooldownSeconds));
    query.bindValue(QStringLiteral(":created_at"), now);
    query.bindValue(QStringLiteral(":updated_at"), now);

    if (!query.exec()) {
        setLastError(QStringLiteral("新增 ROI 规则失败"), query.lastError().text());
        return false;
    }

    rule->id = query.lastInsertId().toInt();
    rule->rect = rect;
    rule->ruleName = rule->ruleName.trimmed().isEmpty() ? QStringLiteral("ROI") : rule->ruleName.trimmed();
    rule->cooldownSeconds = std::max(0, rule->cooldownSeconds);
    rule->createdAt = now;
    rule->updatedAt = now;
    m_lastError.clear();
    return true;
}

bool RoiRuleRepository::update(const RoiRule &rule)
{
    if (!ensureDatabaseOpen()) {
        return false;
    }
    if (rule.id <= 0) {
        setLastError(QStringLiteral("更新 ROI 规则失败"), QStringLiteral("规则 id 无效"));
        return false;
    }

    const QRectF rect = rule.rect.normalized();
    if (!rect.isValid() || rect.isEmpty()) {
        setLastError(QStringLiteral("更新 ROI 规则失败"), QStringLiteral("ROI 矩形无效"));
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(R"(
        UPDATE roi_rules
        SET source_id = :source_id,
            rule_name = :rule_name,
            enabled = :enabled,
            points_json = :points_json,
            cooldown_seconds = :cooldown_seconds,
            updated_at = :updated_at
        WHERE id = :id
    )"));
    query.bindValue(QStringLiteral(":source_id"), rule.sourceId);
    query.bindValue(QStringLiteral(":rule_name"), rule.ruleName.trimmed().isEmpty() ? QStringLiteral("ROI") : rule.ruleName.trimmed());
    query.bindValue(QStringLiteral(":enabled"), rule.enabled ? 1 : 0);
    query.bindValue(QStringLiteral(":points_json"), rectToJson(rect));
    query.bindValue(QStringLiteral(":cooldown_seconds"), std::max(0, rule.cooldownSeconds));
    query.bindValue(QStringLiteral(":updated_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":id"), rule.id);

    if (!query.exec()) {
        setLastError(QStringLiteral("更新 ROI 规则失败"), query.lastError().text());
        return false;
    }

    m_lastError.clear();
    return true;
}

bool RoiRuleRepository::remove(int id)
{
    if (!ensureDatabaseOpen()) {
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM roi_rules WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id);

    if (!query.exec()) {
        setLastError(QStringLiteral("删除 ROI 规则失败"), query.lastError().text());
        return false;
    }

    m_lastError.clear();
    return true;
}

bool RoiRuleRepository::removeBySource(int sourceId)
{
    if (!ensureDatabaseOpen()) {
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM roi_rules WHERE source_id = :source_id"));
    query.bindValue(QStringLiteral(":source_id"), sourceId);

    if (!query.exec()) {
        setLastError(QStringLiteral("清空视频源 ROI 规则失败"), query.lastError().text());
        return false;
    }

    m_lastError.clear();
    return true;
}

QString RoiRuleRepository::lastError() const
{
    return m_lastError;
}

QVector<RoiRule> RoiRuleRepository::loadByQuery(QSqlQuery &query, const QString &context)
{
    QVector<RoiRule> rules;
    if (!query.exec()) {
        setLastError(context, query.lastError().text());
        return rules;
    }

    while (query.next()) {
        const RoiRule rule = readRule(query);
        if (!rule.rect.isValid() || rule.rect.isEmpty()) {
            qWarning().noquote() << QStringLiteral("跳过无效 ROI 规则，id=%1").arg(rule.id);
            continue;
        }

        rules.append(rule);
    }

    m_lastError.clear();
    return rules;
}

bool RoiRuleRepository::ensureDatabaseOpen()
{
    if (m_database.isValid() && m_database.isOpen()) {
        return true;
    }

    setLastError(QStringLiteral("数据库操作失败"), QStringLiteral("数据库未打开"));
    return false;
}

void RoiRuleRepository::setLastError(const QString &context, const QString &detail)
{
    m_lastError = QStringLiteral("%1: %2").arg(context, detail);
    qWarning().noquote() << m_lastError;
}
