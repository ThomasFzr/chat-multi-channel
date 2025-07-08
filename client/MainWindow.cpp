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
#include <QMessageBox>
#include <QApplication>
#include <QDialog>
#include <QFrame>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    clientSocket(new ClientSocket(this)),
    userWidget(nullptr),
    userIcon(nullptr),
    userNameLabel(nullptr),
    userSettingsButton(nullptr),
    roleUserLineEdit(nullptr),
    roleComboBox(nullptr),
    setRoleButton(nullptr),
    unbanUserLineEdit(nullptr),
    unbanUserButton(nullptr),
    listBannedButton(nullptr)
{
    ui->setupUi(this);

    // Demander le pseudo à l'utilisateur au démarrage
    bool ok;
    QString username;
    do {
        username = QInputDialog::getText(this, "Connexion au Chat", 
                                       "Entrez votre pseudo :", 
                                       QLineEdit::Normal, "", &ok);
        if (!ok) {
            // L'utilisateur a annulé, fermer l'application
            QApplication::quit();
            return;
        }
        if (username.isEmpty()) {
            QMessageBox::warning(this, "Pseudo requis", "Veuillez entrer un pseudo valide.");
        }
    } while (username.isEmpty());
    
    currentUser = username;
    
    // Demander si l'utilisateur veut être admin
    QMessageBox::StandardButton reply = QMessageBox::question(this, 
        "Rôle utilisateur", 
        "Voulez-vous vous connecter en tant qu'administrateur ?",
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        currentRole = "admin";
        requestedAdminRole = true;
    } else {
        currentRole = "user";
        requestedAdminRole = false;
    }

    // Configuration des salons - ajouter les salons par défaut temporairement
    // Ils seront remplacés par la liste du serveur
    ui->roomListWidget->addItem("general");
    ui->roomListWidget->addItem("gaming");
    ui->roomListWidget->addItem("music");
    ui->roomListWidget->addItem("dev");
    ui->roomListWidget->setCurrentRow(0);
    currentRoom = "general"; // Définir le salon par défaut
    
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

    // Connecter le signal de connexion pour envoyer la demande admin après connexion
    connect(clientSocket, &ClientSocket::connected, this, [this]() {
        qDebug() << "CLIENT: Connecté au serveur";
        
        // Demander la liste des salons disponibles
        QJsonObject roomListObj;
        roomListObj["type"] = "request_room_list";
        roomListObj["user"] = currentUser;
        QJsonDocument roomListDoc(roomListObj);
        clientSocket->sendMessage(roomListDoc.toJson(QJsonDocument::Compact));
        qDebug() << "CLIENT: Demande de liste des salons envoyée";
        
        // NE PAS envoyer la demande admin ici car currentRoom n'est pas encore défini
        // La demande admin sera envoyée après avoir reçu la liste des salons
    });

    clientSocket->connectToServer("10.8.0.13", 1234);

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
                QAction *deleteAction = menu.addAction("🗑️ Supprimer le message");
                QAction *infoAction = menu.addAction("ℹ️ Infos du message");
                
                QAction *selected = menu.exec(ui->chatListWidget->viewport()->mapToGlobal(pos));
                
                if (selected == deleteAction) {
                    int index = ui->chatListWidget->row(item);
                    QString messageText = item->text();
                    
                    // Confirmation avant suppression
                    int ret = QMessageBox::question(this, "Supprimer le message", 
                                                   "Êtes-vous sûr de vouloir supprimer ce message ?\n\n" + messageText,
                                                   QMessageBox::Yes | QMessageBox::No);
                    
                    if (ret == QMessageBox::Yes) {
                        QJsonObject obj;
                        obj["type"] = "delete_message";
                        obj["room"] = currentRoom;
                        obj["user"] = currentUser;
                        obj["message_content"] = messageText;
                        obj["client_index"] = index;  // Index côté client pour référence
                        QJsonDocument doc(obj);
                        clientSocket->sendMessage(doc.toJson(QJsonDocument::Compact));
                        qDebug() << "Demande de suppression envoyée pour message:" << messageText;
                    }
                }
                else if (selected == infoAction) {
                    int index = ui->chatListWidget->row(item);
                    QString messageText = item->text();
                    QMessageBox::information(this, "Information du message", 
                                           "Index: " + QString::number(index) + "\nContenu: " + messageText);
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
    qDebug() << "CLIENT: Message reçu:" << msg;
    
    // Gestion des messages JSON (role_update, user_banned, user_unbanned)
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(msg.toUtf8(), &err);
    if (err.error == QJsonParseError::NoError && doc.isObject()) {
        QJsonObject obj = doc.object();
        QString msgType = obj["type"].toString();
        
        qDebug() << "CLIENT: Message JSON parsé avec succès, type:" << msgType;
        qDebug() << "CLIENT: Contenu JSON complet:" << doc.toJson(QJsonDocument::Compact);
        
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
        else if (msgType == "room_list") {
            QJsonArray roomArray = obj["rooms"].toArray();
            
            qDebug() << "CLIENT: Reçu liste des salons:" << roomArray;
            
            // Sauvegarder le salon actuellement sélectionné
            QString selectedRoom = currentRoom;
            
            // Vider la liste actuelle et ajouter les salons reçus
            ui->roomListWidget->clear();
            for (const QJsonValue& roomValue : roomArray) {
                QString roomName = roomValue.toString();
                ui->roomListWidget->addItem(roomName);
            }
            
            // Essayer de re-sélectionner le salon précédent, sinon prendre le premier
            bool foundPreviousRoom = false;
            for (int i = 0; i < ui->roomListWidget->count(); ++i) {
                if (ui->roomListWidget->item(i)->text() == selectedRoom) {
                    ui->roomListWidget->setCurrentRow(i);
                    foundPreviousRoom = true;
                    break;
                }
            }
            
            if (!foundPreviousRoom && ui->roomListWidget->count() > 0) {
                ui->roomListWidget->setCurrentRow(0);
                currentRoom = ui->roomListWidget->currentItem()->text();
            }
            
            // Se connecter au salon si on en a un
            if (ui->roomListWidget->count() > 0 && !currentRoom.isEmpty()) {
                expectingHistory = true;
                clientSocket->joinRoom(currentRoom, currentUser);
                ui->chatListWidget->clear();
                
                // Vérifier si l'utilisateur est banni dans ce salon
                if (roomBanStatus.value(currentRoom, false)) {
                    ui->chatListWidget->addItem("⚠️ VOUS ÊTES BANNI DE CE SALON ⚠️");
                    ui->chatListWidget->addItem("💬 Vous pouvez voir les messages mais pas en envoyer");
                }
                
                // Maintenant que currentRoom est défini, envoyer la demande admin si nécessaire
                if (requestedAdminRole) {
                    QJsonObject obj;
                    obj["type"] = "request_admin";
                    obj["user"] = currentUser;
                    obj["room"] = currentRoom;
                    QJsonDocument doc(obj);
                    QString jsonString = doc.toJson(QJsonDocument::Compact);
                    qDebug() << "CLIENT: JSON à envoyer:" << jsonString;
                    clientSocket->sendMessage(jsonString.toUtf8());
                    qDebug() << "CLIENT: Demande de rôle admin envoyée pour" << currentUser;
                }
                
                qDebug() << "CLIENT: Connecté au salon:" << currentRoom;
            }
            
            return;
        }
        else if (msgType == "room_created") {
            QString newRoom = obj["newRoom"].toString();
            QString creator = obj["creator"].toString();
            
            qDebug() << "CLIENT: Reçu notification room_created pour salon:" << newRoom << "par" << creator;
            
            // Ajouter le nouveau salon à la liste s'il n'existe pas déjà
            bool roomExists = false;
            for (int i = 0; i < ui->roomListWidget->count(); ++i) {
                if (ui->roomListWidget->item(i)->text() == newRoom) {
                    roomExists = true;
                    break;
                }
            }
            
            if (!roomExists) {
                ui->roomListWidget->addItem(newRoom);
                ui->chatListWidget->addItem("🏠 Nouveau salon créé: " + newRoom + " (par " + creator + ")");
                qDebug() << "CLIENT: Nouveau salon ajouté à la liste:" << newRoom;
            } else {
                qDebug() << "CLIENT: Salon" << newRoom << "existe déjà dans la liste";
            }
            
            return;
        }
        else if (msgType == "room_exists") {
            QString room = obj["room"].toString();
            ui->chatListWidget->addItem("❌ Le salon '" + room + "' existe déjà !");
            return;
        }
        else if (msgType == "message_deleted") {
            QString messageContent = obj["message_content"].toString();
            QString room = obj["room"].toString();
            QString deletedBy = obj["deleted_by"].toString();
            
            // Supprimer le message de la liste si on est dans le bon salon
            if (room == currentRoom) {
                // Chercher et supprimer le message dans la liste
                for (int i = 0; i < ui->chatListWidget->count(); ++i) {
                    QListWidgetItem *item = ui->chatListWidget->item(i);
                    if (item && item->text() == messageContent) {
                        delete ui->chatListWidget->takeItem(i);
                        qDebug() << "Message supprimé de l'interface:" << messageContent;
                        break;
                    }
                }
                
                // Supprimer aussi de l'historique local
                roomMessages[room].removeAll(messageContent);
                
                // Afficher une notification de suppression
                ui->chatListWidget->addItem("🗑️ Message supprimé par " + deletedBy);
            }
            return;
        }
        else if (msgType == "message_deleted") {
            QString deletedContent = obj["message_content"].toString();
            QString room = obj["room"].toString();
            QString deletedBy = obj["deleted_by"].toString();
            
            // Supprimer le message de la liste affichée si c'est le salon actuel
            if (room == currentRoom) {
                // Chercher le message par son contenu et le supprimer
                for (int i = 0; i < ui->chatListWidget->count(); i++) {
                    QListWidgetItem *item = ui->chatListWidget->item(i);
                    if (item && item->text() == deletedContent) {
                        // Supprimer l'item de la liste
                        ui->chatListWidget->takeItem(i);
                        delete item;
                        
                        // Mettre à jour l'historique local
                        roomMessages[currentRoom].removeOne(deletedContent);
                        
                        qDebug() << "Message supprimé:" << deletedContent << "par" << deletedBy;
                        break; // Supprimer seulement la première occurrence
                    }
                }
            }
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
    userWidget->setFixedHeight(60); // Hauteur fixe pour s'assurer que tout est visible
    userWidget->setStyleSheet("QWidget#userWidget { background-color: #2C2F33; border-radius: 8px; padding: 8px; margin: 4px; }");
    
    QHBoxLayout *userLayout = new QHBoxLayout(userWidget);
    userLayout->setContentsMargins(8, 8, 8, 8);
    userLayout->setSpacing(10); // Espacement entre les éléments
    
    // Icône utilisateur
    userIcon = new QLabel(userWidget);
    userIcon->setObjectName("userIcon");
    userIcon->setFixedSize(35, 35);
    userIcon->setAlignment(Qt::AlignCenter);
    userIcon->setText("U"); // Lettre U pour User par défaut
    userIcon->setStyleSheet("QLabel { background-color: #7289DA; border-radius: 17px; color: white; font-size: 14px; font-weight: bold; }");
    
    // Conteneur pour nom et rôle
    QVBoxLayout *userInfoLayout = new QVBoxLayout();
    userInfoLayout->setSpacing(2);
    userInfoLayout->setContentsMargins(0, 0, 0, 0);
    
    // Nom d'utilisateur
    userNameLabel = new QLabel(currentUser + " (" + currentRole + ")", userWidget);
    userNameLabel->setObjectName("userNameLabel");
    userNameLabel->setStyleSheet("color: #fff; font-weight: bold; font-size: 16px; margin: 0px; padding: 5px; background-color: rgba(114, 137, 218, 0.2); border-radius: 3px;");
    userNameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    userNameLabel->setMinimumWidth(150); // Largeur minimum
    userNameLabel->setWordWrap(false);
    
    // Bouton paramètres utilisateur
    userSettingsButton = new QPushButton("⚙️", userWidget);
    userSettingsButton->setObjectName("userSettingsButton");
    userSettingsButton->setFixedSize(28, 28);
    userSettingsButton->setStyleSheet("QPushButton { background-color: #23272A; border: 1px solid #7289DA; border-radius: 14px; color: white; font-size: 12px; } QPushButton:hover { background-color: #7289DA; }");
    userSettingsButton->setToolTip("Paramètres utilisateur");
    
    connect(userSettingsButton, &QPushButton::clicked, this, [this]() {
        // Menu contextuel pour les paramètres utilisateur
        QMenu menu(this);
        
        // Option pour afficher les informations (lecture seule)
        QAction *infoAction = menu.addAction("📋 Informations du profil");
        connect(infoAction, &QAction::triggered, this, [this]() {
            // Affichage simple avec QMessageBox
            QString info = QString("=== PROFIL UTILISATEUR ===\n\n") +
                          QString("👤 Nom: %1\n").arg(currentUser) +
                          QString("🔑 Rôle: %1\n").arg(currentRole == "admin" ? "Administrateur" : "Utilisateur") +
                          QString("🏠 Salon: %1\n").arg(currentRoom) +
                          QString("🟢 Statut: En ligne");
            
            QMessageBox::information(this, "Informations du profil", info);
        });
        
        menu.addSeparator();
        
        // Option pour se déconnecter
        QAction *disconnectAction = menu.addAction("🚪 Se déconnecter");
        connect(disconnectAction, &QAction::triggered, this, [this]() {
            // Optionnel: ajouter une logique de déconnexion
            close();
        });
        
        // Afficher le menu près du bouton
        menu.exec(userSettingsButton->mapToGlobal(QPoint(0, userSettingsButton->height())));
    });
    
    userInfoLayout->addWidget(userNameLabel);
    
    // Debug: s'assurer que le label est visible
    userNameLabel->show();
    qDebug() << "CLIENT: UserNameLabel créé avec texte:" << userNameLabel->text();
    
    userLayout->addWidget(userIcon, 0); // 0 = pas d'étirement
    userLayout->addLayout(userInfoLayout, 1); // 1 = étirement pour prendre plus de place
    userLayout->addWidget(userSettingsButton, 0); // 0 = pas d'étirement
    
    // Insérer le widget utilisateur en haut de l'interface
    ui->verticalLayout->insertWidget(0, userWidget);
}

void MainWindow::updateUserInterface()
{
    // Mise à jour du label de rôle
    ui->roleLabel->setText("Rôle : " + currentRole);
    
    if (userNameLabel) {
        userNameLabel->setText(currentUser + " (" + currentRole + ")");
        userNameLabel->show(); // Forcer l'affichage
        userNameLabel->update(); // Forcer le rafraîchissement
        qDebug() << "CLIENT: Mise à jour du label utilisateur:" << userNameLabel->text();
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
            userIcon->setStyleSheet("QLabel { background-color: #ff6b6b; border-radius: 17px; color: white; font-size: 14px; font-weight: bold; }");
            userIcon->setText("A"); // A pour Admin
        } else {
            userIcon->setStyleSheet("QLabel { background-color: #7289DA; border-radius: 17px; color: white; font-size: 14px; font-weight: bold; }");
            userIcon->setText("U"); // U pour User
        }
    }
}
