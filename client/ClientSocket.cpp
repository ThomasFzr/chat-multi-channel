#include "ClientSocket.h"

#include <QObject>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

ClientSocket::ClientSocket(QObject *parent) :
    QObject(parent), socket(new QTcpSocket(this)) {
    connect(socket, &QTcpSocket::readyRead, this, &ClientSocket::onReadyRead);
    connect(socket, &QTcpSocket::connected, this, [this]() {
        qDebug() << "CLIENT SOCKET: Connexion établie, émission du signal connected";
        emit connected();
    });
}

void ClientSocket::connectToServer(const QString &host, quint16 port) {
    qDebug() << "CLIENT SOCKET: Tentative de connexion à" << host << ":" << port;
    socket->connectToHost(host, port);
}

void ClientSocket::sendMessage(const QByteArray &msg) {
    socket->write(msg);
}

void ClientSocket::onReadyRead() {
    QByteArray data = socket->readAll();
    emit messageReceived(QString::fromUtf8(data));
}

void ClientSocket::joinRoom(const QString &room, const QString &user) {
    QJsonObject obj;
    obj["type"] = "join";
    obj["room"] = room;
    obj["user"] = user;
    QJsonDocument doc(obj);
    socket->write(doc.toJson(QJsonDocument::Compact));
}

void ClientSocket::leaveRoom(const QString &room, const QString &user) {
    QJsonObject obj;
    obj["type"] = "leave";
    obj["room"] = room;
    obj["user"] = user;
    QJsonDocument doc(obj);
    socket->write(doc.toJson(QJsonDocument::Compact));
}

void ClientSocket::sendChatMessage(const QString &room, const QString &user, const QString &text) {
    QJsonObject obj;
    obj["type"] = "message";
    obj["room"] = room;
    obj["user"] = user;
    obj["text"] = text;
    QJsonDocument doc(obj);
    socket->write(doc.toJson(QJsonDocument::Compact));
}
