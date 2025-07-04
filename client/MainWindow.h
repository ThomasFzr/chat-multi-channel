#pragma once

#include <QMainWindow>

class ClientSocket;  // tu dois déclarer ta classe socket

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
    void onMessageReceived(const QString &msg); // 👈 ajoute cette déclaration

private:
    Ui::MainWindow *ui;
    ClientSocket *clientSocket; // 👈 ajoute ce membre
};
