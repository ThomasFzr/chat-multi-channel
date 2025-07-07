#include "ChatServer.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QJsonArray>

void saveHistory(const QMap<QString, QStringList>& history) {
    QJsonObject root;
    for (auto it = history.begin(); it != history.end(); ++it) {
        QJsonArray arr;
        for (const QString& msg : it.value()) arr.append(msg);
        root[it.key()] = arr;
    }
    QFile file("history.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

QMap<QString, QStringList> loadHistory() {
    QMap<QString, QStringList> history;
    QFile file("history.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject root = doc.object();
        for (auto it = root.begin(); it != root.end(); ++it) {
            QStringList msgs;
            for (const QJsonValue& v : it.value().toArray()) msgs.append(v.toString());
            history[it.key()] = msgs;
        }
        file.close();
    }
    return history;
}

ChatServer::ChatServer(QObject *parent) :
    QObject(parent), server(new QTcpServer(this)) {
    roomHistory = loadHistory();
    connect(server, &QTcpServer::newConnection, this, &ChatServer::onNewConnection);
}

void ChatServer::start(quint16 port) {
    server->listen(QHostAddress::Any, port);
}

void ChatServer::onNewConnection() {
    QTcpSocket *client = server->nextPendingConnection();
    connect(client, &QTcpSocket::readyRead, [=] {
        QByteArray data = client->readAll();
        handleMessage(client, data);
    });
    connect(client, &QTcpSocket::disconnected, [=] {
        if (clients.contains(client)) {
            QString room = clients.value(client);
            rooms[room].removeOne(client);
            clients.remove(client);
        }
        client->deleteLater();
    });
}

void ChatServer::handleMessage(QTcpSocket *client, const QByteArray &data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();
    QString room = obj["room"].toString();
    QString user = obj["user"].toString();

    if (type == "join") {
        clients[client] = room;
        rooms[room].append(client);
        for(const QString &msg : roomHistory[room]) {
            client->write((msg + "\n").toUtf8());
        }
        for (QTcpSocket *other : rooms[room]) {
            if (other != client) {
                other->write((user + " joined " + room + "\n").toUtf8());
            }
        }
    } else if (type == "message") {
        QString text = obj["text"].toString();
        QString fullMsg = user + ": " + text;
        roomHistory[room].append(fullMsg);
        saveHistory(roomHistory);
        broadcast(room, fullMsg);
    } else if (type == "leave") {
        rooms[room].removeOne(client);
        broadcast(room, user + " left " + room);
    }
}

void ChatServer::broadcast(const QString &room, const QString &message) {
    QByteArray msg = (message + "\n").toUtf8();
    for (QTcpSocket *client : rooms[room]) {
        client->write(msg);
    }
}
