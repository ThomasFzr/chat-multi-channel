cmake_minimum_required(VERSION 3.16)

project(chat-multi-channel VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ajout de Network ici :
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

set(PROJECT_SOURCES
    Main.cpp
        MainWindow.cpp
        MainWindow.h
        MainWindow.ui
        ChatController.h
        ChatController.cpp
        NetworkManager.h
        NetworkManager.cpp
        MessageFactory.h
        Message.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(chat-multi-channel
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(chat-multi-channel SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(chat-multi-channel
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(chat-multi-channel PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(chat-multi-channel)
endif()
