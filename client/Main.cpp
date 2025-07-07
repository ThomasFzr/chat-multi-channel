#include <QApplication>
#include <QMainWindow>
#include <QPalette>
#include <QStyleFactory>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(44,47,51));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(35,39,42));
    darkPalette.setColor(QPalette::AlternateBase, QColor(44,47,51));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(35,39,42));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Highlight, QColor(114,137,218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);

    a.setPalette(darkPalette);
    a.setStyle(QStyleFactory::create("Fusion"));

    MainWindow w;
    w.showMaximized();
    return a.exec();
}
