#pragma once

#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>

#include "message.h"


class CompilerServer : public QTcpServer {
  Q_OBJECT
public:
  explicit CompilerServer(QObject *parent = nullptr) : QTcpServer(parent) {}

protected:
  void incomingConnection(qintptr fd) override;

private slots:
  void onReadyRead();

private:
  QMap<QTcpSocket*, QByteArray> buffers_;

  void handle(QTcpSocket *sock, const Message &msg);
  void send(QTcpSocket *sock, const Message &msg);
};
