#pragma once
#include <QString>

class Message {
public:
    virtual ~Message() {}
    virtual QString serialize() const = 0;
};

class TextMessage : public Message {
public:
    TextMessage(const QString& txt) : text(txt) {}
    QString serialize() const override { return text; }

private:
    QString text;
};
