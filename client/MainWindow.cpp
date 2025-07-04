#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "ClientSocket.h"
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    clientSocket(new ClientSocket(this))
{
    ui->setupUi(this);

    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(clientSocket, &ClientSocket::messageReceived, this, &MainWindow::onMessageReceived);

    clientSocket->connectToServer("127.0.0.1", 1234);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onSendClicked()
{
    QString text = ui->messageLineEdit->text();
    clientSocket->sendMessage(text.toUtf8());
    ui->messageLineEdit->clear();
    ui->chatTextEdit->append(text);
}

void MainWindow::onMessageReceived(const QString &msg)
{
    ui->chatTextEdit->append(msg);
}
