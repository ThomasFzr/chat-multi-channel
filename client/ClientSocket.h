# pragma once

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <QByteArray>
#include <QtGlobal>

class ClientSocket : public QObject
{
    Q_OBJECT

public:
    explicit ClientSocket(QObject *parent = nullptr);

    void connectToServer(const QString &host, quint16 port);
    void sendMessage(const QByteArray &msg);
    void joinRoom(const QString &room, const QString &user);
    void leaveRoom(const QString &room, const QString &user);
    void sendChatMessage(const QString &room, const QString &user, const QString &text);

signals:
    void messageReceived(const QString &msg);

private slots:
    void onReadyRead();

private:
    QTcpSocket *socket;
};
