#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "ClientSocket.h"
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QMap>
#include <QStringList>
#include <QMenu>
#include <QInputDialog>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QComboBox>
#include <QToolButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    clientSocket(new ClientSocket(this)),
    userWidget(nullptr),
    userIcon(nullptr),
    userNameLabel(nullptr),
    userRoleCombo(nullptr),
    userSettingsButton(nullptr),
    roleUserLineEdit(nullptr),
    roleComboBox(nullptr),
    setRoleButton(nullptr),
    unbanUserLineEdit(nullptr),
    unbanUserButton(nullptr),
    listBannedButton(nullptr)
{
    ui->setupUi(this);

    // Configuration des salons
    ui->roomListWidget->addItem("general");
    ui->roomListWidget->addItem("gaming");
    ui->roomListWidget->addItem("music");
    ui->roomListWidget->addItem("dev");
    ui->roomListWidget->setCurrentRow(0);
    
    // Setup du widget utilisateur
    setupUserInterface();
    
    connect(ui->roomListWidget, &QListWidget::currentTextChanged, this, [this](const QString &room){
        currentRoom = room;
        expectingHistory = true;
        clientSocket->joinRoom(currentRoom, currentUser);
        ui->chatListWidget->clear();
        
        // Vérifier si l'utilisateur est banni dans ce salon
        if (roomBanStatus.value(currentRoom, false)) {
            ui->chatListWidget->addItem("⚠️ VOUS ÊTES BANNI DE CE SALON ⚠️");
            ui->chatListWidget->addItem("💬 Vous pouvez voir les messages mais pas en envoyer");
        }
    });

    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);
    connect(clientSocket, &ClientSocket::messageReceived, this, &MainWindow::onMessageReceived);

    clientSocket->connectToServer("10.8.0.13", 1234);
    clientSocket->joinRoom(currentRoom, currentUser);

    // Si c'est user1, le définir comme admin par défaut
    if (currentUser == "user1") {
        currentRole = "admin";
    }

    // Mise à jour initiale de l'interface
    updateUserInterface();

    connect(ui->createRoomButton, &QPushButton::clicked, this, [this]() {
        if (currentRole == "admin") {
            bool ok;
            QString newRoom = QInputDialog::getText(this, "Créer un salon", "Nom du salon :", QLineEdit::Normal, "", &ok);
            if (ok && !newRoom.isEmpty()) {
                QJsonObject obj;
                obj["type"] = "create_room";
                obj["room"] = currentRoom;
                obj["user"] = currentUser;
                obj["newRoom"] = newRoom;
                QJsonDocument doc(obj);
                clientSocket->sendMessage(doc.toJson(QJsonDocument::Compact));
            }
        }
    });

    connect(ui->banUserButton, &QPushButton::clicked, this, [this]() {
        if (currentRole == "admin") {
            QString target = ui->banUserLineEdit->text();
            if (!target.isEmpty()) {
                QJsonObject obj;
                obj["type"] = "ban";
                obj["room"] = currentRoom;
                obj["user"] = currentUser;
                obj["target"] = target;
                QJsonDocument doc(obj);
                clientSocket->sendMessage(doc.toJson(QJsonDocument::Compact));
                ui->banUserLineEdit->clear();
                qDebug() << "Demande de bannissement envoyée pour:" << target;
            }
        } else {
            qDebug() << "Bannissement refusé: utilisateur n'est pas admin";
        }
    });

    // Menu contextuel pour suppression de message (admin)
    ui->chatListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->chatListWidget, &QListWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        if (currentRole == "admin") {
            QListWidgetItem *item = ui->chatListWidget->itemAt(pos);
            if (item) {
                QMenu menu(this);
                QAction *deleteAction = menu.addAction("Supprimer le message");
                QAction *selected = menu.exec(ui->chatListWidget->viewport()->mapToGlobal(pos));
                if (selected == deleteAction) {
                    int index = ui->chatListWidget->row(item);
                    QJsonObject obj;
                    obj["type"] = "delete_message";
                    obj["room"] = currentRoom;
                    obj["user"] = currentUser;
                    obj["index"] = index;
                    QJsonDocument doc(obj);
                    clientSocket->sendMessage(doc.toJson(QJsonDocument::Compact));
                }
            }
        }
    });

    // Ajout bouton et champ pour changer le rôle d'un utilisateur (admin)
    QHBoxLayout *roleLayout = new QHBoxLayout();
    roleUserLineEdit = new QLineEdit();
    roleUserLineEdit->setObjectName("roleUserLineEdit");
    roleUserLineEdit->setPlaceholderText("Utilisateur à promouvoir/dégrader (admin)");
    roleUserLineEdit->setStyleSheet("background-color: #23272A; color: #fff; border-radius: 5px; padding: 6px;");
    roleComboBox = new QComboBox();
    roleComboBox->setObjectName("roleComboBox");
    roleComboBox->addItem("admin");
    roleComboBox->addItem("user");
    setRoleButton = new QToolButton();
    setRoleButton->setObjectName("setRoleButton");
    setRoleButton->setText("Définir rôle");
    setRoleButton->setStyleSheet("background-color: #43b581; color: #fff; border-radius: 5px; padding: 6px 16px;");
    roleLayout->addWidget(roleUserLineEdit);
    roleLayout->addWidget(roleComboBox);
    roleLayout->addWidget(setRoleButton);
    ui->verticalLayout->insertLayout(2, roleLayout);
    roleUserLineEdit->setVisible(currentRole == "admin");
    roleComboBox->setVisible(currentRole == "admin");
    setRoleButton->setVisible(currentRole == "admin");
    connect(setRoleButton, &QToolButton::clicked, this, [this]() {
        if (currentRole == "admin") {
            QString target = roleUserLineEdit->text();
            QString newRole = roleComboBox->currentText();
            if (!target.isEmpty()) {
                QJsonObject obj;
                obj["type"] = "set_role";
                obj["room"] = currentRoom;
                obj["user"] = currentUser;
                obj["target"] = target;
                obj["role"] = newRole;
                QJsonDocument doc(obj);
                clientSocket->sendMessage(doc.toJson(QJsonDocument::Compact));
                roleUserLineEdit->clear();
                qDebug() << "Demande de changement de rôle envoyée:" << target << "vers" << newRole;
            }
        } else {
            qDebug() << "Changement de rôle refusé: utilisateur n'est pas admin";
        }
    });

    // Ajout de l'interface débannir
    QHBoxLayout *unbanLayout = new QHBoxLayout();
    QLineEdit *unbanUserLineEdit = new QLineEdit();
    unbanUserLineEdit->setObjectName("unbanUserLineEdit");
    unbanUserLineEdit->setPlaceholderText("Utilisateur à débannir (admin)");
    unbanUserLineEdit->setStyleSheet("background-color: #23272A; color: #fff; border-radius: 5px; padding: 6px;");
    QPushButton *unbanUserButton = new QPushButton("Débannir");
    unbanUserButton->setObjectName("unbanUserButton");
    unbanUserButton->setStyleSheet("background-color: #43b581; color: #fff; border-radius: 5px; padding: 6px 16px;");
    QPushButton *listBannedButton = new QPushButton("📋 Liste bannis");
    listBannedButton->setObjectName("listBannedButton");
    listBannedButton->setStyleSheet("background-color: #f39c12; color: #fff; border-radius: 5px; padding: 6px 16px;");
    unbanLayout->addWidget(unbanUserLineEdit);
    unbanLayout->addWidget(unbanUserButton);
    unbanLayout->addWidget(listBannedButton);
    ui->verticalLayout->insertLayout(3, unbanLayout);
    unbanUserLineEdit->setVisible(currentRole == "admin");
    unbanUserButton->setVisible(currentRole == "admin");
    listBannedButton->setVisible(currentRole == "admin");
    
    connect(unbanUserButton, &QPushButton::clicked, this, [this, unbanUserLineEdit]() {
        if (currentRole == "admin") {
            QString target = unbanUserLineEdit->text();
            if (!target.isEmpty()) {
                QJsonObject obj;
                obj["type"] = "unban";
                obj["room"] = currentRoom;
                obj["user"] = currentUser;
                obj["target"] = target;
                QJsonDocument doc(obj);
                clientSocket->sendMessage(doc.toJson(QJsonDocument::Compact));
                unbanUserLineEdit->clear();
            }
        }
    });
    
    connect(listBannedButton, &QPushButton::clicked, this, [this]() {
        if (currentRole == "admin") {
            QJsonObject obj;
            obj["type"] = "get_banned_users";
            obj["room"] = currentRoom;
            obj["user"] = currentUser;
            QJsonDocument doc(obj);
            clientSocket->sendMessage(doc.toJson(QJsonDocument::Compact));
        }
    });
    
    // Stockage des références pour updateUserInterface
    this->unbanUserLineEdit = unbanUserLineEdit;
    this->unbanUserButton = unbanUserButton;
    this->listBannedButton = listBannedButton;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onSendClicked()
{
    // Vérifier si l'utilisateur est banni dans ce salon
    if (roomBanStatus.value(currentRoom, false)) {
        ui->chatListWidget->addItem("❌ Vous ne pouvez pas envoyer de messages car vous êtes banni de ce salon");
        ui->chatListWidget->addItem("💡 Changez de salon pour continuer à chatter");
        return;
    }
    
    QString text = ui->messageLineEdit->text();
    if (!text.isEmpty()) {
        clientSocket->sendChatMessage(currentRoom, currentUser, text);
        ui->messageLineEdit->clear();
    }
}

void MainWindow::onMessageReceived(const QString &msg)
{
    // Gestion des messages JSON (role_update, user_banned, user_unbanned)
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8(), &err);
    if (err.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject obj = doc.object();
        QString msgType = obj["type"].toString();
        
        if (msgType == "role_update") {
            QString newRole = obj["role"].toString();
            QString target = obj.value("target").toString();
            
            // Si le message est pour nous (pas de target ou target = notre nom)
            if (target.isEmpty() || target == currentUser) {
                currentRole = newRole;
                updateUserInterface();
                qDebug() << "Rôle mis à jour pour" << currentUser << ":" << currentRole;
            }
            return;
        }
        else if (msgType == "user_banned") {
            QString target = obj["target"].toString();
            QString room = obj["room"].toString();
            
            // Afficher une notification visuelle
            if (target == currentUser) {
                ui->chatListWidget->addItem("❌ VOUS AVEZ ÉTÉ BANNI DE CE SALON ❌");
                ui->chatListWidget->addItem("💡 Vous pouvez changer de salon pour continuer à utiliser le chat");
            } else {
                ui->chatListWidget->addItem("🚫 " + target + " a été banni du salon");
            }
            return;
        }
        else if (msgType == "you_are_banned") {
            QString room = obj["room"].toString();
            
            // Marquer ce salon comme banni pour cet utilisateur
            roomBanStatus[room] = true;
            
            // Afficher un message clair
            ui->chatListWidget->addItem("❌ VOUS AVEZ ÉTÉ BANNI DE CE SALON ❌");
            ui->chatListWidget->addItem("💡 Changez de salon pour continuer à chatter");
            ui->chatListWidget->addItem("ℹ️ Vos messages ne seront pas envoyés dans ce salon");
            
            // Mettre à jour l'interface si c'est le salon actuel
            if (room == currentRoom) {
                updateUserInterface();
            }
            
            return;
        }
        else if (msgType == "user_unbanned") {
            QString target = obj["target"].toString();
            QString room = obj["room"].toString();
            
            // Afficher une notification visuelle
            if (target == currentUser) {
                // Réactiver l'envoi pour ce salon
                roomBanStatus[room] = false;
                ui->chatListWidget->addItem("✅ VOUS AVEZ ÉTÉ DÉBANNI DE CE SALON ✅");
                ui->chatListWidget->addItem("🎉 Vous pouvez maintenant envoyer des messages !");
                
                // Mettre à jour l'interface si c'est le salon actuel
                if (room == currentRoom) {
                    updateUserInterface();
                }
            } else {
                ui->chatListWidget->addItem("✅ " + target + " a été débanni du salon");
            }
            return;
        }
        else if (msgType == "banned_users_list") {
            QJsonArray bannedArray = obj["banned"].toArray();
            QString room = obj["room"].toString();
            
            QStringList bannedUsers;
            for (const QJsonValue &value : bannedArray) {
                bannedUsers.append(value.toString());
            }
            
            QString message;
            if (bannedUsers.isEmpty()) {
                message = "📋 Aucun utilisateur banni dans " + room;
            } else {
                message = "📋 Utilisateurs bannis dans " + room + ":\n• " + bannedUsers.join("\n• ");
            }
            
            // Afficher dans un dialogue
            QInputDialog::getMultiLineText(this, "Liste des utilisateurs bannis", 
                                         "Salon: " + room, message);
            return;
        }
        
        // Si c'est un autre type de message JSON, on l'ignore (ne pas l'afficher)
        return;
    }
    
    // Filtrer les lignes vides et les messages JSON bruts
    QStringList messages = msg.split("\n", Qt::SkipEmptyParts);
    QStringList filteredMessages;
    
    for (const QString &message : messages) {
        // Ignorer les messages JSON bruts qui commencent par { et finissent par }
        QString trimmed = message.trimmed();
        if (trimmed.startsWith("{") && trimmed.endsWith("}")) {
            // Vérifier si c'est vraiment du JSON
            QJsonParseError jsonErr;
            QJsonDocument::fromJson(trimmed.toUtf8(), &jsonErr);
            if (jsonErr.error == QJsonParseError::NoError) {
                continue; // Ignorer ce message JSON
            }
        }
        filteredMessages.append(message);
    }
    
    if (expectingHistory) {
        // On vient de faire un join, on remplace l'historique local
        roomMessages[currentRoom] = filteredMessages;
        ui->chatListWidget->clear();
        for(const QString &m : filteredMessages)
            ui->chatListWidget->addItem(m);
        expectingHistory = false;
    } else {
        // Message normal, on ajoute
        for(const QString &m : filteredMessages) {
            // Ajoute uniquement si le message n'est pas déjà dans l'historique local
            if (!roomMessages[currentRoom].contains(m)) {
                roomMessages[currentRoom].append(m);
                ui->chatListWidget->addItem(m);
            }
        }
    }
}

void MainWindow::setupUserInterface()
{
    // Vérifier que l'UI est bien initialisée
    if (!ui || !ui->verticalLayout) {
        qDebug() << "UI not properly initialized";
        return;
    }

    // Setup du widget utilisateur avec icône, pseudo et bouton de paramètres
    userWidget = new QWidget(this);
    userWidget->setObjectName("userWidget");
    userWidget->setStyleSheet("QWidget#userWidget { background-color: #2C2F33; border-radius: 8px; padding: 8px; margin: 4px; }");
    
    QHBoxLayout *userLayout = new QHBoxLayout(userWidget);
    userLayout->setContentsMargins(8, 8, 8, 8);
    
    // Icône utilisateur
    userIcon = new QLabel(userWidget);
    userIcon->setObjectName("userIcon");
    userIcon->setFixedSize(40, 40);
    userIcon->setAlignment(Qt::AlignCenter);
    userIcon->setText("👤"); // Emoji utilisateur par défaut
    userIcon->setStyleSheet("QLabel { background-color: #7289DA; border-radius: 20px; color: white; font-size: 20px; }");
    
    // Conteneur pour nom et rôle
    QVBoxLayout *userInfoLayout = new QVBoxLayout();
    userInfoLayout->setSpacing(2);
    
    // Nom d'utilisateur
    userNameLabel = new QLabel(currentUser + " (" + currentRole + ")", userWidget);
    userNameLabel->setObjectName("userNameLabel");
    userNameLabel->setStyleSheet("color: #fff; font-weight: bold; font-size: 14px;");
    
    // Combo box pour le rôle - TOUJOURS VISIBLE pour le test
    userRoleCombo = new QComboBox(userWidget);
    userRoleCombo->setObjectName("userRoleCombo");
    userRoleCombo->addItem("user");
    userRoleCombo->addItem("admin");
    userRoleCombo->setCurrentText(currentRole);
    userRoleCombo->setStyleSheet("QComboBox { background-color: #23272A; color: #fff; border: 1px solid #7289DA; border-radius: 4px; padding: 4px; } QComboBox::drop-down { border: none; } QComboBox::down-arrow { image: none; }");
    
    connect(userRoleCombo, &QComboBox::currentTextChanged, this, &MainWindow::onUserRoleChanged);
    
    // Bouton paramètres utilisateur
    userSettingsButton = new QPushButton("⚙️", userWidget);
    userSettingsButton->setObjectName("userSettingsButton");
    userSettingsButton->setFixedSize(30, 30);
    userSettingsButton->setStyleSheet("QPushButton { background-color: #23272A; border: 1px solid #7289DA; border-radius: 15px; color: white; font-size: 14px; } QPushButton:hover { background-color: #7289DA; }");
    userSettingsButton->setToolTip("Paramètres utilisateur");
    
    connect(userSettingsButton, &QPushButton::clicked, this, [this]() {
        // Menu contextuel pour les paramètres utilisateur
        QMenu menu(this);
        
        // Option pour changer le pseudo
        QAction *changeNameAction = menu.addAction("Changer le pseudo");
        connect(changeNameAction, &QAction::triggered, this, [this]() {
            bool ok;
            QString newName = QInputDialog::getText(this, "Changer le pseudo", "Nouveau pseudo :", QLineEdit::Normal, currentUser, &ok);
            if (ok && !newName.isEmpty() && newName != currentUser) {
                currentUser = newName;
                userNameLabel->setText(currentUser + " (" + currentRole + ")");
                // Rejoindre le salon avec le nouveau nom
                clientSocket->joinRoom(currentRoom, currentUser);
            }
        });
        
        menu.addSeparator();
        
        // Option pour forcer le changement de rôle
        QAction *forceRoleAction = menu.addAction("Forcer le rôle admin");
        connect(forceRoleAction, &QAction::triggered, this, [this]() {
            currentRole = "admin";
            userRoleCombo->setCurrentText(currentRole);
            updateUserInterface();
        });
        
        menu.addSeparator();
        
        // Option pour afficher les informations
        QAction *infoAction = menu.addAction("Informations");
        connect(infoAction, &QAction::triggered, this, [this]() {
            QString info = QString("Utilisateur: %1\nRôle: %2\nSalon: %3").arg(currentUser, currentRole, currentRoom);
            QInputDialog::getMultiLineText(this, "Informations utilisateur", "Informations:", info);
        });
        
        // Afficher le menu près du bouton
        menu.exec(userSettingsButton->mapToGlobal(QPoint(0, userSettingsButton->height())));
    });
    
    userInfoLayout->addWidget(userNameLabel);
    userInfoLayout->addWidget(userRoleCombo);
    
    userLayout->addWidget(userIcon);
    userLayout->addLayout(userInfoLayout);
    userLayout->addStretch();
    userLayout->addWidget(userSettingsButton);
    
    // Insérer le widget utilisateur en haut de l'interface
    ui->verticalLayout->insertWidget(0, userWidget);
}

void MainWindow::updateUserInterface()
{
    // Mise à jour du label de rôle
    ui->roleLabel->setText("Rôle : " + currentRole);
    
    // Mise à jour de la combo box de rôle utilisateur et du nom
    if (userRoleCombo) {
        userRoleCombo->setCurrentText(currentRole);
        // ComboBox toujours visible pour permettre les tests
    }
    
    if (userNameLabel) {
        userNameLabel->setText(currentUser + " (" + currentRole + ")");
    }
    
    // Mettre à jour le placeholder du champ de message selon le statut de ban
    if (roomBanStatus.value(currentRoom, false)) {
        ui->messageLineEdit->setPlaceholderText("❌ Vous êtes banni de ce salon - Changez de salon");
        ui->messageLineEdit->setStyleSheet("background-color: #4a1a1a; color: #ff6b6b; border-radius: 5px; padding: 6px;");
    } else {
        ui->messageLineEdit->setPlaceholderText("Écrire un message...");
        ui->messageLineEdit->setStyleSheet("background-color: #23272A; color: #fff; border-radius: 5px; padding: 6px;");
    }
    
    // Affiche/masque les éléments admin
    ui->createRoomButton->setVisible(currentRole == "admin");
    ui->banUserLineEdit->setVisible(currentRole == "admin");
    ui->banUserButton->setVisible(currentRole == "admin");
    
    // Affiche/masque les widgets admin de gestion des rôles
    if (roleUserLineEdit) roleUserLineEdit->setVisible(currentRole == "admin");
    if (roleComboBox) roleComboBox->setVisible(currentRole == "admin");
    if (setRoleButton) setRoleButton->setVisible(currentRole == "admin");
    
    // Affiche/masque les widgets admin de débannissement
    if (unbanUserLineEdit) unbanUserLineEdit->setVisible(currentRole == "admin");
    if (unbanUserButton) unbanUserButton->setVisible(currentRole == "admin");
    if (listBannedButton) listBannedButton->setVisible(currentRole == "admin");
    
    // Mise à jour du style de l'icône utilisateur en fonction du rôle
    if (userIcon) {
        if (currentRole == "admin") {
            userIcon->setStyleSheet("QLabel { background-color: #ff6b6b; border-radius: 20px; color: white; font-size: 20px; }");
            userIcon->setText("👑"); // Couronne pour admin
        } else {
            userIcon->setStyleSheet("QLabel { background-color: #7289DA; border-radius: 20px; color: white; font-size: 20px; }");
            userIcon->setText("👤"); // Utilisateur normal
        }
    }
}

void MainWindow::onUserRoleChanged(const QString &newRole)
{
    if (newRole != currentRole) {
        // Mettre à jour le rôle localement d'abord
        currentRole = newRole;
        updateUserInterface();
        
        // Envoyer la demande de changement de rôle au serveur
        QJsonObject obj;
        obj["type"] = "set_role";
        obj["room"] = currentRoom;
        obj["user"] = currentUser;
        obj["target"] = currentUser;
        obj["role"] = newRole;
        QJsonDocument doc(obj);
        clientSocket->sendMessage(doc.toJson(QJsonDocument::Compact));
        
        qDebug() << "Changement de rôle demandé:" << currentUser << "vers" << newRole;
    }
}
