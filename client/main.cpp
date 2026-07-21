#include <QApplication>

#include "mainwindow.h"
#include "theme.h"

int main(int argc, char *argv[]) {
  QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
  qputenv("QT_QPA_PLATFORM", "xcb");
  qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--no-sandbox --disable-gpu");
  QApplication a(argc, argv);
  a.setStyleSheet(getBlackFoxQSS());
  MainWindow w;
  w.show();
  return QCoreApplication::exec();
}
