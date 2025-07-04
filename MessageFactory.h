#pragma once
#include "Message.h"

class MessageFactory {
public:
    static Message* createTextMessage(const QString& text) {
        return new TextMessage(text);
    }
};
