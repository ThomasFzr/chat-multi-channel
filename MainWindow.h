#pragma once

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ChatController;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void onMessageReceived(const QString& msg);

private slots:
    void onSendButtonClicked();

private:
    Ui::MainWindow *ui;
    ChatController* controller;
};
