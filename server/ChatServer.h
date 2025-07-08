#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QList>
#include <QString>
#include <QByteArray>
#include <QSet>
#include <QJsonObject>

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
    void sendJson(QTcpSocket *client, const QJsonObject &obj);
    void handleSingleMessage(QTcpSocket *client, const QJsonObject &obj);
    void sendRoomList(QTcpSocket *client);

    QTcpServer *server;
    QMap<QTcpSocket*, QString> clients; // client -> room
    QMap<QTcpSocket*, QString> clientUsers; // client -> username
    QMap<QString, QList<QTcpSocket*>> rooms;
    QMap<QString, QStringList> roomHistory;
    QMap<QString, QString> userRoles;
    QMap<QString, QSet<QString>> roomBans;
};

#endif
