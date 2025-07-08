cmake_minimum_required(VERSION 3.16)
project(ChatApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Widgets Network Svg REQUIRED)

# Server executable
add_executable(server
    server/Main.cpp
    server/ChatServer.cpp
    server/ChatServer.h
)
target_link_libraries(server Qt6::Core Qt6::Network)

# Client executable
add_executable(client
    client/Main.cpp
    client/MainWindow.cpp
    client/MainWindow.h
    client/ClientSocket.cpp
    client/ClientSocket.h
    client/MessageFactory.cpp
    client/MessageFactory.h
    client/MainWindow.ui
    client/resources.qrc
)
target_link_libraries(client Qt6::Core Qt6::Widgets Qt6::Network Qt6::Svg)
