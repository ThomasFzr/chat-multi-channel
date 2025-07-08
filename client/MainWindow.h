#pragma once

#include <QMainWindow>
#include <QMap>
#include <QStringList>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QWidget>
#include <QLineEdit>
#include <QToolButton>

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
    void updateUserInterface();
    void setupUserInterface();
    
    Ui::MainWindow *ui;
    ClientSocket *clientSocket;
    QString currentRoom = "general";
    QString currentUser;
    QMap<QString, QStringList> roomMessages;
    bool expectingHistory = false;
    QString currentRole = "user";
    bool requestedAdminRole = false;
    QMap<QString, bool> roomBanStatus; // Track ban status per room
    
    // Widgets utilisateur
    QWidget *userWidget;
    QLabel *userIcon;
    QLabel *userNameLabel;
    QPushButton *userSettingsButton;
    
    // Widgets admin pour gestion des rôles
    QLineEdit *roleUserLineEdit;
    QComboBox *roleComboBox;
    QToolButton *setRoleButton;
    
    // Widgets admin pour débannir
    QLineEdit *unbanUserLineEdit;
    QPushButton *unbanUserButton;
    QPushButton *listBannedButton;
};
