#include "main_window.hpp"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName("Video Speed Changer");
    QApplication::setApplicationDisplayName("Video Speed Changer");

    MainWindow w;

    QIcon icon(":/assets/icon.png");
    w.setWindowIcon(icon);

    // w.setWindowTitle("My App Title");
    w.resize(800, 600);
    w.show();
    return a.exec();
}
