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

void saveRooms(const QMap<QString, QList<QTcpSocket*>>& rooms) {
    QJsonArray roomArray;
    for (auto it = rooms.begin(); it != rooms.end(); ++it) {
        roomArray.append(it.key());
    }
    QJsonObject root;
    root["rooms"] = roomArray;
    QFile file("rooms.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

QStringList loadRooms() {
    QStringList roomList;
    QFile file("rooms.json");
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject root = doc.object();
        QJsonArray roomArray = root["rooms"].toArray();
        for (const QJsonValue& room : roomArray) {
            roomList.append(room.toString());
        }
        file.close();
    }
    
    // Toujours s'assurer que les salons par défaut existent
    QStringList defaultRooms = {"general", "gaming", "music", "dev"};
    for (const QString& defaultRoom : defaultRooms) {
        if (!roomList.contains(defaultRoom)) {
            roomList.prepend(defaultRoom); // Ajouter au début pour les garder en premier
        }
    }
    
    // Si aucun salon n'était dans le fichier, créer seulement les salons par défaut
    if (roomList.isEmpty()) {
        roomList = defaultRooms;
    }
    
    return roomList;
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
    
    // Charger les salons existants
    QStringList roomList = loadRooms();
    for (const QString& roomName : roomList) {
        rooms[roomName] = QList<QTcpSocket*>();
        roomBans[roomName] = QSet<QString>();
        // S'assurer que l'historique existe pour chaque salon
        if (!roomHistory.contains(roomName)) {
            roomHistory[roomName] = QStringList();
        }
    }
    
    // Sauvegarder immédiatement les salons pour s'assurer que les salons par défaut sont persistés
    saveRooms(rooms);
    
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
    qDebug() << "SERVEUR: Données brutes reçues:" << data;
    
    // Séparer les messages JSON (ils peuvent être concaténés)
    QString dataStr = QString::fromUtf8(data);
    
    // Essayer de séparer les messages JSON en trouvant les accolades fermantes
    QStringList jsonMessages;
    int start = 0;
    int braceCount = 0;
    
    for (int i = 0; i < dataStr.length(); ++i) {
        if (dataStr[i] == '{') {
            braceCount++;
        } else if (dataStr[i] == '}') {
            braceCount--;
            if (braceCount == 0) {
                // Fin d'un message JSON complet
                QString jsonMsg = dataStr.mid(start, i - start + 1);
                jsonMessages.append(jsonMsg);
                start = i + 1;
            }
        }
    }
    
    // Traiter chaque message JSON séparément
    for (const QString &jsonStr : jsonMessages) {
        if (jsonStr.trimmed().isEmpty()) continue;
        
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError) {
            qDebug() << "SERVEUR: Erreur parsing JSON:" << error.errorString() << "dans" << jsonStr;
            continue;
        }
        
        QJsonObject obj = doc.object();
        QString type = obj["type"].toString();
        QString room = obj["room"].toString();
        QString user = obj["user"].toString();
        
        qDebug() << "SERVEUR: Message reçu - Type:" << type << "User:" << user << "Room:" << room;
        
        // Traiter le message
        handleSingleMessage(client, obj);
    }
}

void ChatServer::handleSingleMessage(QTcpSocket *client, const QJsonObject &obj) {
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
            QString currentRoom = obj["room"].toString(); // Salon depuis lequel l'admin crée
            if (!rooms.contains(newRoom) && !newRoom.isEmpty()) {
                rooms[newRoom] = QList<QTcpSocket*>();
                roomHistory[newRoom] = QStringList();
                roomBans[newRoom] = QSet<QString>(); // Initialiser les bannissements pour ce salon
                
                // Sauvegarder la liste des salons
                saveRooms(rooms);
                
                qDebug() << "Nouveau salon créé:" << newRoom << "par" << user;
                
                // Notifier tous les clients connectés du nouveau salon
                QJsonObject roomCreatedMsg{{"type", "room_created"}, {"newRoom", newRoom}, {"creator", user}};
                qDebug() << "SERVEUR: Envoi notification room_created pour salon:" << newRoom << "à" << clients.size() << "clients";
                for (QTcpSocket *c : clients.keys()) {
                    sendJson(c, roomCreatedMsg);
                    qDebug() << "SERVEUR: Notification envoyée à client" << clients[c];
                }
                
                // Informer dans le salon actuel que le salon a été créé
                if (!currentRoom.isEmpty() && rooms.contains(currentRoom)) {
                    broadcast(currentRoom, user + " a créé le salon " + newRoom);
                }
            } else if (rooms.contains(newRoom)) {
                // Notifier le créateur que le salon existe déjà
                QJsonObject errorMsg{{"type", "room_exists"}, {"room", newRoom}};
                sendJson(client, errorMsg);
            }
        } else {
            qDebug() << "Création de salon refusée:" << user << "n'est pas admin";
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
            QString messageContent = obj["message_content"].toString();
            QString room = obj["room"].toString();
            
            // Rechercher le message par son contenu
            int index = -1;
            for (int i = 0; i < roomHistory[room].size(); i++) {
                if (roomHistory[room][i] == messageContent) {
                    index = i;
                    break;
                }
            }
            
            if (index >= 0) {
                roomHistory[room].removeAt(index);
                saveHistory(roomHistory);
                
                qDebug() << "Message supprimé par" << user << "dans" << room << "- contenu:" << messageContent;
                
                // Notifier tous les clients de la suppression avec le contenu du message
                QJsonObject deleteMsg{{"type", "message_deleted"}, {"message_content", messageContent}, {"room", room}, {"deleted_by", user}};
                for (QTcpSocket *c : rooms[room]) {
                    sendJson(c, deleteMsg);
                }
                
                broadcast(room, "📝 Un message a été supprimé par " + user);
            } else {
                qDebug() << "Tentative de suppression échouée - message non trouvé:" << messageContent;
            }
        } else {
            qDebug() << "Tentative de suppression échouée:" << user << "n'est pas admin";
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
    } else if (type == "request_admin") {
        // Demande de rôle admin par un nouvel utilisateur
        QString requestUser = obj["user"].toString();
        qDebug() << "SERVEUR: Demande de rôle admin reçue pour:" << requestUser;
        
        // Afficher tous les rôles existants
        qDebug() << "SERVEUR: Rôles actuels:";
        for (auto it = userRoles.begin(); it != userRoles.end(); ++it) {
            qDebug() << "  -" << it.key() << ":" << it.value();
        }
        
        // Accepter automatiquement si c'est le premier utilisateur ou si aucun admin n'existe
        bool hasAdmin = false;
        for (auto it = userRoles.begin(); it != userRoles.end(); ++it) {
            if (it.value() == "admin") {
                hasAdmin = true;
                qDebug() << "SERVEUR: Admin trouvé:" << it.key();
                break;
            }
        }
        
        if (!hasAdmin) {
            userRoles[requestUser] = "admin";
            qDebug() << "SERVEUR: Premier admin assigné:" << requestUser;
        } else {
            userRoles[requestUser] = "user";
            qDebug() << "SERVEUR: Utilisateur normal assigné:" << requestUser << "(admin existe déjà)";
        }
        
        // Notifier le client de son rôle final
        QJsonObject roleMsg{{"type", "role_update"}, {"role", userRoles[requestUser]}};
        sendJson(client, roleMsg);
        qDebug() << "SERVEUR: Rôle final envoyé à" << requestUser << ":" << userRoles[requestUser];
        
    } else if (type == "get_banned_users") {
        if (userRoles.value(user) == "admin") {
            QJsonArray bannedArray;
            for (const QString &bannedUser : roomBans[room]) {
                bannedArray.append(bannedUser);
            }
            QJsonObject response{{"type", "banned_users_list"}, {"banned", bannedArray}, {"room", room}};
            sendJson(client, response);
        }
    } else if (type == "request_room_list") {
        // Envoyer la liste des salons disponibles
        sendRoomList(client);
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

void ChatServer::sendRoomList(QTcpSocket *client) {
    QJsonArray roomArray;
    for (auto it = rooms.begin(); it != rooms.end(); ++it) {
        roomArray.append(it.key());
    }
    QJsonObject roomListMsg{{"type", "room_list"}, {"rooms", roomArray}};
    sendJson(client, roomListMsg);
    qDebug() << "SERVEUR: Liste des salons envoyée:" << roomArray;
}
