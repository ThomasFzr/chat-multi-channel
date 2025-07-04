#include <QCoreApplication>
#include "ChatServer.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    ChatServer server;
    server.start(1234);
    return a.exec();
}
