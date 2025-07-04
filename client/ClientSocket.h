class ClientSocket : public QObject {
    Q_OBJECT
public:
    explicit ClientSocket(QObject *parent = nullptr);
    void connectToServer(const QString &host, quint16 port);
    void sendMessage(const QByteArray &msg);
signals:
    void messageReceived(const QString &msg);
private slots:
    void onReadyRead();
private:
    QTcpSocket *socket;
};