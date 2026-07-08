#include "compilerserver.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

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
  auto *sock = qobject_cast<QTcpSocket *>(sender());
  buffers_[sock].append(sock->readAll());

  Message msg;
  while (Message::tryDeserialize(buffers_[sock], msg)) {
    handle(sock, msg);
  }
}

void CompilerServer::handle(QTcpSocket *sock, const Message &msg) {
  // qDebug() << "Got flag:" << msg.flag;
  Message reply;

  if (msg.flag == "load") {
    QString path = msg.payload["path"].toString();
    QFile f(path);
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
    QString path = msg.payload["path"].toString();
    QString content = msg.payload["content"].toString();
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
      f.write(content.toUtf8());
      reply.flag = "output";
      reply.payload["ok"] = true;
    } else {
      reply.flag = "error";
      reply.payload["message"] = f.errorString();
    }
    send(sock, reply);

  } else if (msg.flag == "ls") {
    QDir directory(msg.payload["directory"].toString());

    if (!directory.exists()) {
      Message reply;
      reply.flag = "error";
      reply.payload["message"] = "directory does not exist";
      send(sock, reply);
      return;
    }

    QFileInfoList dirList = directory.entryInfoList(QDir::Dirs | QDir::Files |
                                                    QDir::NoDotAndDotDot);

    QStringList names;
    for (const QFileInfo &fi : dirList) {
      QString name = fi.fileName();
      if (fi.isDir()) {
        name += "/";
      }
      names << name;
    }

    Message reply;
    reply.flag = "output";
    reply.payload["ok"] = true;
    reply.payload["content"] = names.join(" ");
    send(sock, reply);

  } else {
    reply.flag = "error";
    reply.payload["message"] = "unknown flag";
    send(sock, reply);
  }
}

void CompilerServer::send(QTcpSocket *sock, const Message &msg) {
  sock->write(msg.serialize());
}
