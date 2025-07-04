#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ChatController.h"
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), controller(new ChatController(this))
{
    ui->setupUi(this);

    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::onSendButtonClicked);
    connect(controller, &ChatController::messageReceived, this, &MainWindow::onMessageReceived);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::onSendButtonClicked() {
    QString text = ui->messageLineEdit->text();
    controller->sendMessage(text);
}

void MainWindow::onMessageReceived(const QString& msg) {
    ui->chatTextEdit->append(msg);
}
