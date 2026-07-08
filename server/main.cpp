#include <QCoreApplication>
#include "compilerserver.h"
#include <QDebug>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    CompilerServer server;
    if (!server.listen(QHostAddress::Any, 5000)) {
        qCritical() << "Cannot listen:" << server.errorString();
        return 1;
    }
    qDebug() << "Server listening on port 5000";
    return app.exec();
}
