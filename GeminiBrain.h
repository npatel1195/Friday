#pragma once
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class GeminiBrain : public QObject {
    Q_OBJECT
public:
    explicit GeminiBrain(QObject *parent = nullptr);
    void setApiKey(const QString &key);
    void sendMessage(const QString &text);
signals:
    void responseReceived(const QString &text);
    void actionTriggered(const QString &type, const QString &val);
private:
    QNetworkAccessManager *manager;
    QJsonArray history;
    QString m_apiKey;
    void handleReply(QNetworkReply *reply);
};
