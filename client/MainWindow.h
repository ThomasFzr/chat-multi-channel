class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
private slots:
    void onSendClicked();
    void onMessageReceived(const QString &msg);
private:
    Ui::MainWindow *ui;
    ClientSocket *clientSocket;
};