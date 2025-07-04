#include "NetworkManager.h"

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent), socket(new QTcpSocket(this))
{
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    socket->connectToHost("127.0.0.1", 1234);
}

void NetworkManager::send(const QString &data) {
    socket->write(data.toUtf8());
}

void NetworkManager::onReadyRead() {
    QString data = socket->readAll();
    emit dataReceived(data);
}
