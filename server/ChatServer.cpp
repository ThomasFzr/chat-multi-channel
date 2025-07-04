ChatServer::ChatServer(QObject *parent) :
    QObject(parent), server(new QTcpServer(this)) {
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
        QString room = clients.value(client);
        rooms[room].removeOne(client);
        clients.remove(client);
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
        broadcast(room, user + " joined " + room);
    } else if (type == "message") {
        QString text = obj["text"].toString();
        broadcast(room, user + ": " + text);
    } else if (type == "leave") {
        rooms[room].removeOne(client);
        broadcast(room, user + " left " + room);
    }
}

void ChatServer::broadcast(const QString &room, const QString &message) {
    QByteArray msg = message.toUtf8();
    for (QTcpSocket *client : rooms[room]) {
        client->write(msg);
    }
}