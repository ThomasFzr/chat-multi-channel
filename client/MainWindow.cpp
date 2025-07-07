#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "ClientSocket.h"
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QMap>
#include <QStringList>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    clientSocket(new ClientSocket(this))
{
    ui->setupUi(this);

    ui->roomListWidget->addItem("general");
    ui->roomListWidget->addItem("gaming");
    ui->roomListWidget->addItem("music");
    ui->roomListWidget->addItem("dev");
    ui->roomListWidget->setCurrentRow(0);
    connect(ui->roomListWidget, &QListWidget::currentTextChanged, this, [this](const QString &room){
        currentRoom = room;
        expectingHistory = true;
        clientSocket->joinRoom(currentRoom, currentUser);
        ui->chatListWidget->clear();
        // On n'affiche pas l'historique ici, il sera affiché à la réception
    });

    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(clientSocket, &ClientSocket::messageReceived, this, &MainWindow::onMessageReceived);

    clientSocket->connectToServer("10.8.0.13", 1234);
    clientSocket->joinRoom(currentRoom, currentUser);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onSendClicked()
{
    QString text = ui->messageLineEdit->text();
    clientSocket->sendChatMessage(currentRoom, currentUser, text);
    ui->messageLineEdit->clear();
}

void MainWindow::onMessageReceived(const QString &msg)
{
    // Découpe les messages reçus sur les sauts de ligne
    QStringList messages = msg.split("\n", Qt::SkipEmptyParts);
    if (expectingHistory) {
        // On vient de faire un join, on remplace l'historique local
        roomMessages[currentRoom] = messages;
        ui->chatListWidget->clear();
        for(const QString &m : messages)
            ui->chatListWidget->addItem(m);
        expectingHistory = false;
    } else {
        // Message normal, on ajoute
        for(const QString &m : messages) {
            // Ajoute uniquement si le message n'est pas déjà dans l'historique local
            if (!roomMessages[currentRoom].contains(m)) {
                roomMessages[currentRoom].append(m);
                ui->chatListWidget->addItem(m);
            }
        }
    }
}
