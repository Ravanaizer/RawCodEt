// #include "mainwindow.h"

// #include <QApplication>

// int main(int argc, char *argv[]) {
//     qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--no-sandbox --disable-gpu");
//     QApplication a(argc, argv);
//     MainWindow w;
//     w.show();
//     return QCoreApplication::exec();
// }


#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    return app.exec();
}
