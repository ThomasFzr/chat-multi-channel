#include "ChatController.h"
#include "NetworkManager.h"
#include "MessageFactory.h"

ChatController::ChatController(QObject *parent)
    : QObject(parent), network(new NetworkManager(this))
{
    connect(network, &NetworkManager::dataReceived, this, &ChatController::messageReceived);
}

void ChatController::sendMessage(const QString& text) {
    auto msg = MessageFactory::createTextMessage(text);
    network->send(msg->serialize());
}
