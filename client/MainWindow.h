#pragma once

#include <QMainWindow>
#include <QMap>
#include <QStringList>

class ClientSocket;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSendClicked();
    void onMessageReceived(const QString &msg);

private:
    Ui::MainWindow *ui;
    ClientSocket *clientSocket;
    QString currentRoom = "general";
    QString currentUser = "user1";
    QMap<QString, QStringList> roomMessages;
    bool expectingHistory = false;
};
