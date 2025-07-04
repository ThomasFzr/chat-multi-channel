#pragma once
#include <QObject>

class NetworkManager;

class ChatController : public QObject {
    Q_OBJECT
public:
    explicit ChatController(QObject* parent = nullptr);
    void sendMessage(const QString& text);

signals:
    void messageReceived(const QString& msg);

private:
    NetworkManager* network;
};
