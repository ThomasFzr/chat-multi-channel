class ChatServer : public QObject {
    Q_OBJECT
public:
    ChatServer(QObject *parent = nullptr);
    void start(quint16 port);
private slots:
    void onNewConnection();
private:
    QTcpServer *server;
    QMap<QTcpSocket*, QString> clients; 
    QMap<QString, QList<QTcpSocket*>> rooms; 
    void handleMessage(QTcpSocket *client, const QByteArray &data);
    void broadcast(const QString &room, const QString &message);
};