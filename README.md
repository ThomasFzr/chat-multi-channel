# 📡 ChatApp - Application de messagerie instantanée (Qt/C++)

ChatApp est une application de communication en temps réel développée en **C++ avec Qt**. Elle permet aux utilisateurs de rejoindre des salons, d'échanger via messages texte, images, vidéos, et de participer à des discussions vocales. Elle intègre également une gestion des utilisateurs et des outils d'administration.

---

## 🚀 Fonctionnalités principales

### Utilisateurs
- Connexion avec authentification
- Changement de statut (en ligne, occupé, absent)
- Liste des salons disponibles
- Rejoindre / Quitter un salon
- Envoi de messages texte, images, vidéos
- Consultation de l'historique des messages
- Discussion audio en temps réel

### Administrateurs
- Création / Suppression de salons
- Mute, kick, ban des utilisateurs
- Suppression de messages
- Suppression de bots (même hors ligne)

---

## 🛠️ Technologies utilisées

- **C++**
- **Qt 5/6 (Qt Widgets & Qt Multimedia)**
- **Qt Network** pour la communication client-serveur
- **QWebSocket** (si chat en temps réel via WebSocket)
- Architecture **MVC**

---

## 📦 Installation

### Prérequis
- Qt Creator (ou Qt installé manuellement)
- Qt version 5.15 ou supérieure
- CMake (si en dehors de Qt Creator)

### 📥 Étapes
git clone https://github.com/ThomasFzr/chat-multi-channel

cd ChatApp 

**Avec Qt Creator**
```bash
Ouvrir ChatApp.pro

Compiler le projet

Exécuter depuis l’IDE

Compiler le projet

Exécuter depuis l’IDE
```

**En ligne de commande**
```bash
qmake
make
./ChatApp
```
### 📁 Structure du projet
```
ChatApp/
├── CMakeLists.txt
├── client/
│   ├── Forms/
│   │   └── client/
│   │       └── MainWindow.ui
│   ├── Header Files/
│   │   └── client/
│   │       ├── ClientSocket.h
│   │       ├── MainWindow.h
│   │       └── MessageFactory.h
│   ├── Resources/
│   │   └── client/
│   │       └── resources.qrc
│   └── Source Files/
│       └── client/
│           ├── ClientSocket.cpp
│           ├── Main.cpp
│           ├── MainWindow.cpp
│           └── MessageFactory.cpp
├── server/
│   ├── Header Files/
│   │   └── server/
│   │       └── ChatServer.h
│   └── Source Files/
│       └── server/
│           ├── ChatServer.cpp
│           └── Main.cpp
```
###  **🧪 Fonctionnalités à venir**
Notifications système

Authentification à deux facteurs (2FA)

messages privé

chat vocal

Système de rôles personnalisés

Réactions aux messages


### **🤝 Auteurs**
 Chikbouni Sofiane - sofiane200206
 
 Foltzer Thomas - ThomasFzr
 
 Meleo Quentin - quentinmel



### 📄 Licence
Ce projet est sous licence MIT. Voir le fichier LICENSE pour plus d'informations.


