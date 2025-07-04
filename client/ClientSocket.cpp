ClientSocket::ClientSocket(QObject *parent) :
    QObject(parent), socket(new QTcpSocket(this)) {
    connect(socket, &QTcpSocket::readyRead, this, &ClientSocket::onReadyRead);
}

void ClientSocket::connectToServer(const QString &host, quint16 port) {
    socket->connectToHost(host, port);
}

void ClientSocket::sendMessage(const QByteArray &msg) {
    socket->write(msg);
}

void ClientSocket::onReadyRead() {
    QByteArray data = socket->readAll();
    emit messageReceived(QString::fromUtf8(data));
}