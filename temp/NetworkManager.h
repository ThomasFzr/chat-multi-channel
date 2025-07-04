#pragma once
#include <QObject>
#include <QTcpSocket>

class NetworkManager : public QObject {
    Q_OBJECT

public:
    explicit NetworkManager(QObject* parent = nullptr);
    void send(const QString& data);

signals:
    void dataReceived(const QString& data);

private slots:
    void onReadyRead();

private:
    QTcpSocket* socket;
};
