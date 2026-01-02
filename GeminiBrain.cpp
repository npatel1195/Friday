#include "GeminiBrain.h"
#include <QDebug>
#include <QUrl>

GeminiBrain::GeminiBrain(QObject *parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
}

const QString API_KEY;
void GeminiBrain::setApiKey(const QString &key) {
    m_apiKey = key;
    qDebug() << "🔑 API Key set successfully.";
}


void GeminiBrain::sendMessage(const QString &text) {
    qDebug() << "🧠 Sending to AI:" << text;

    if (m_apiKey.isEmpty()) {
        emit responseReceived("I am missing my API Key, sir.");
        return;
    }

    QNetworkRequest req(QUrl("https://api.openai.com/v1/chat/completions"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());

    // 1. SYSTEM PROMPT
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = "You are Friday. "
                           "If user says OPEN/PLAY, use 'open_app' or 'open_website'. "
                           "If user says CLOSE/STOP/TERMINATE an app, use 'close_app'. "
                           "Be concise.";

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = text;

    QJsonArray messages;
    messages.append(systemMsg);
    messages.append(userMsg);

    // 2. DEFINE OPEN TOOL
    QJsonObject openAppFunc;
    openAppFunc["name"] = "open_app";
    openAppFunc["description"] = "Opens an app";
    QJsonObject openParams;
    openParams["type"] = "object";
    QJsonObject openProps;
    openProps["appName"] = QJsonObject{{"type", "string"}};
    openParams["properties"] = openProps;
    openAppFunc["parameters"] = openParams;

    // 3. DEFINE CLOSE TOOL (NEW)
    QJsonObject closeAppFunc;
    closeAppFunc["name"] = "close_app";
    closeAppFunc["description"] = "Closes/Terminates a running application";
    QJsonObject closeParams;
    closeParams["type"] = "object";
    QJsonObject closeProps;
    closeProps["appName"] = QJsonObject{{"type", "string"}};
    closeParams["properties"] = closeProps;
    closeAppFunc["parameters"] = closeParams;

    // 4. DEFINE WEB TOOL
    QJsonObject openWebFunc;
    openWebFunc["name"] = "open_website";
    openWebFunc["description"] = "Opens a website";
    QJsonObject webParams;
    webParams["type"] = "object";
    QJsonObject webProps;
    webProps["url"] = QJsonObject{{"type", "string"}};
    webParams["properties"] = webProps;
    openWebFunc["parameters"] = webParams;

    QJsonArray tools;
    tools.append(QJsonObject{{"type", "function"}, {"function", openAppFunc}});
    tools.append(QJsonObject{{"type", "function"}, {"function", closeAppFunc}}); // Add to list
    tools.append(QJsonObject{{"type", "function"}, {"function", openWebFunc}});

    QJsonObject root;
    root["model"] = "gpt-4o-mini";
    root["messages"] = messages;
    root["tools"] = tools;
    root["tool_choice"] = "auto";

    QByteArray data = QJsonDocument(root).toJson();
    QNetworkReply *reply = manager->post(req, data);

    connect(reply, &QNetworkReply::finished, this, [this, reply](){
        handleReply(reply);
    });
}

void GeminiBrain::handleReply(QNetworkReply *reply) {
    if(reply->error()) {
        qDebug() << "❌ ERROR:" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray rawData = reply->readAll();
    QJsonObject resp = QJsonDocument::fromJson(rawData).object();

    if(resp.contains("choices")) {
        QJsonArray choices = resp["choices"].toArray();
        if(!choices.isEmpty()) {
            QJsonObject message = choices[0].toObject()["message"].toObject();

            if(message.contains("tool_calls")) {
                QJsonArray toolCalls = message["tool_calls"].toArray();
                for(const QJsonValue &val : toolCalls) {
                    QJsonObject tool = val.toObject();
                    QJsonObject func = tool["function"].toObject();
                    QString name = func["name"].toString();
                    QString argsStr = func["arguments"].toString();
                    QJsonObject args = QJsonDocument::fromJson(argsStr.toUtf8()).object();

                    qDebug() << "⚡ EXECUTE:" << name;

                    if(name == "open_app") emit actionTriggered("open", args["appName"].toString());
                    if(name == "open_website") emit actionTriggered("web", args["url"].toString());

                    // NEW: TRIGGER CLOSE
                    if(name == "close_app") emit actionTriggered("close", args["appName"].toString());
                }
                emit responseReceived("Done.");
            }
            else if(message.contains("content")) {
                QString text = message["content"].toString();
                if(!text.isEmpty()) emit responseReceived(text);
            }
        }
    }
    reply->deleteLater();
}
