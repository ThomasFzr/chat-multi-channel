#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QList>
#include <QString>
#include <QByteArray>

class ChatServer : public QObject
{
    Q_OBJECT

public:
    explicit ChatServer(QObject *parent = nullptr);
    void start(quint16 port);

private slots:
    void onNewConnection();
    void handleMessage(QTcpSocket *client, const QByteArray &data);

private:
    void broadcast(const QString &room, const QString &message);

    QTcpServer *server;
    QMap<QTcpSocket*, QString> clients;
    QMap<QString, QList<QTcpSocket*>> rooms;
    QMap<QString, QStringList> roomHistory;
};

#endif
