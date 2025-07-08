#include "ChatServer.h"

#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QJsonArray>
#include <QSet>
#include <QDebug>

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
    userRoles["user1"] = "admin";
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
            clientUsers.remove(client);
        }
        client->deleteLater();
    });
}

void ChatServer::sendJson(QTcpSocket *client, const QJsonObject &obj) {
    QJsonDocument doc(obj);
    client->write(doc.toJson(QJsonDocument::Compact) + "\n");
}

void ChatServer::handleMessage(QTcpSocket *client, const QByteArray &data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();
    QString room = obj["room"].toString();
    QString user = obj["user"].toString();

    if (type == "join") {
        if (roomBans[room].contains(user)) {
            client->write("Vous êtes banni de ce salon.\n");
            return;
        }
        clients[client] = room;
        clientUsers[client] = user; // Associer le client à l'utilisateur
        rooms[room].append(client);
        if (!userRoles.contains(user)) userRoles[user] = "user";
        // Envoi du rôle au client
        QJsonObject roleMsg{{"type", "role_update"}, {"role", userRoles[user]}};
        sendJson(client, roleMsg);
        for(const QString &msg : roomHistory[room]) {
            client->write((msg + "\n").toUtf8());
        }
        for (QTcpSocket *other : rooms[room]) {
            if (other != client) {
                other->write((user + " joined " + room + "\n").toUtf8());
            }
        }
    } else if (type == "message") {
        if (roomBans[room].contains(user)) return;
        QString text = obj["text"].toString();
        QString fullMsg = user + ": " + text;
        roomHistory[room].append(fullMsg);
        saveHistory(roomHistory);
        broadcast(room, fullMsg);
    } else if (type == "create_room") {
        if (userRoles.value(user) == "admin") {
            QString newRoom = obj["newRoom"].toString();
            if (!rooms.contains(newRoom)) {
                rooms[newRoom] = QList<QTcpSocket*>();
                roomHistory[newRoom] = QStringList();
                broadcast(room, user + " a créé le salon " + newRoom);
            }
        }
    } else if (type == "ban") {
        if (userRoles.value(user) == "admin") {
            QString target = obj["target"].toString();
            roomBans[room].insert(target);
            qDebug() << "Bannissement:" << target << "banni du salon" << room << "par" << user;
            
            // Notifier tous les clients du bannissement
            QJsonObject banMsg{{"type", "user_banned"}, {"target", target}, {"room", room}};
            for (QTcpSocket *c : rooms[room]) {
                sendJson(c, banMsg);
            }
            
            broadcast(room, target + " a été banni du salon par " + user);
            
            // Notifier spécifiquement l'utilisateur banni sans le déconnecter
            for (QTcpSocket *c : clientUsers.keys()) {
                if (clientUsers[c] == target && clients[c] == room) {
                    QJsonObject banNotif{{"type", "you_are_banned"}, {"room", room}};
                    sendJson(c, banNotif);
                    break;
                }
            }
        } else {
            qDebug() << "Tentative de bannissement échouée:" << user << "n'est pas admin";
        }
    } else if (type == "unban") {
        if (userRoles.value(user) == "admin") {
            QString target = obj["target"].toString();
            roomBans[room].remove(target);
            qDebug() << "Débannissement:" << target << "débanni du salon" << room << "par" << user;
            
            // Notifier tous les clients du débannissement
            QJsonObject unbanMsg{{"type", "user_unbanned"}, {"target", target}, {"room", room}};
            for (QTcpSocket *c : rooms[room]) {
                sendJson(c, unbanMsg);
            }
            
            broadcast(room, target + " a été débanni du salon par " + user);
        }
    } else if (type == "delete_message") {
        if (userRoles.value(user) == "admin") {
            int index = obj["index"].toInt();
            if (index >= 0 && index < roomHistory[room].size()) {
                QString deletedMsg = roomHistory[room][index];
                roomHistory[room].removeAt(index);
                saveHistory(roomHistory);
                broadcast(room, "Un message a été supprimé par " + user);
            }
        }
    } else if (type == "set_role") {
        // Gestion du changement de rôle (admin seulement)
        if (userRoles.value(user) == "admin") {
            QString target = obj["target"].toString();
            QString newRole = obj["role"].toString();
            if (newRole == "admin" || newRole == "user") {
                userRoles[target] = newRole;
                qDebug() << "Changement de rôle:" << target << "devient" << newRole;
                
                // Notifier le client cible spécifiquement
                for (QTcpSocket *c : clientUsers.keys()) {
                    if (clientUsers[c] == target) {
                        QJsonObject roleMsg{{"type", "role_update"}, {"role", newRole}};
                        sendJson(c, roleMsg);
                        qDebug() << "Message de rôle envoyé à" << target;
                        break;
                    }
                }
                
                broadcast(room, target + " a maintenant le rôle " + newRole + " (par " + user + ")");
            }
        }
    } else if (type == "get_banned_users") {
        if (userRoles.value(user) == "admin") {
            QJsonArray bannedArray;
            for (const QString &bannedUser : roomBans[room]) {
                bannedArray.append(bannedUser);
            }
            QJsonObject response{{"type", "banned_users_list"}, {"banned", bannedArray}, {"room", room}};
            sendJson(client, response);
        }
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
