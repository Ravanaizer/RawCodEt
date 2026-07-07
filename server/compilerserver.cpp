#include "compilerserver.h"
#include <QFile>
#include <QDebug>

void CompilerServer::incomingConnection(qintptr fd) {
    auto *sock = new QTcpSocket(this);
    sock->setSocketDescriptor(fd);
    buffers_[sock] = QByteArray();

    connect(sock, &QTcpSocket::readyRead, this, &CompilerServer::onReadyRead);
    connect(sock, &QTcpSocket::disconnected, this, [this, sock]() {
        buffers_.remove(sock);
        sock->deleteLater();
    });

    qDebug() << "Client connected from" << sock->peerAddress();
}

void CompilerServer::onReadyRead() {
    auto *sock = qobject_cast<QTcpSocket*>(sender());
    buffers_[sock].append(sock->readAll());

    Message msg;
    while (Message::tryDeserialize(buffers_[sock], msg)) {
        handle(sock, msg);
    }
}

void CompilerServer::handle(QTcpSocket *sock, const Message &msg) {
    qDebug() << "Got flag:" << msg.flag;

    if (msg.flag == "load") {
        QString path = msg.payload["path"].toString();
        QFile f(path);
        Message reply;
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            reply.flag = "output";
            reply.payload["ok"] = true;
            reply.payload["content"] = QString::fromUtf8(f.readAll());
        } else {
            reply.flag = "error";
            reply.payload["message"] = f.errorString();
        }
        send(sock, reply);

    } else if (msg.flag == "save") {
        QString path    = msg.payload["path"].toString();
        QString content = msg.payload["content"].toString();
        QFile f(path);
        Message reply;
        if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            f.write(content.toUtf8());
            reply.flag = "output";
            reply.payload["ok"] = true;
        } else {
            reply.flag = "error";
            reply.payload["message"] = f.errorString();
        }
        send(sock, reply);

    } else {
        Message reply;
        reply.flag = "error";
        reply.payload["message"] = "unknown flag";
        send(sock, reply);
    }
}

void CompilerServer::send(QTcpSocket *sock, const Message &msg) {
    sock->write(msg.serialize());
}
