MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    clientSocket(new ClientSocket(this))
{
    ui->setupUi(this);
    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(clientSocket, &ClientSocket::messageReceived, this, &MainWindow::onMessageReceived);

    clientSocket->connectToServer("127.0.0.1", 1234);
}

void MainWindow::onSendClicked() {
    QString text = ui->messageLineEdit->text();
    QString room = "general"; 
    QString user = "alice";
    QByteArray msg = MessageFactory::createChatMessage(room, user, text);
    clientSocket->sendMessage(msg);
    ui->messageLineEdit->clear();
}

void MainWindow::onMessageReceived(const QString &msg) {
    ui->chatTextEdit->append(msg);
}