#pragma once


#include <QByteArray>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>

class MessageFactory {
public:
    static QByteArray createChatMessage(const QString &room, const QString &user, const QString &text) {
        QJsonObject obj;
        obj["type"] = "message";
        obj["room"] = room;
        obj["user"] = user;
        obj["text"] = text;
        QJsonDocument doc(obj);
        return doc.toJson();
    }
};
