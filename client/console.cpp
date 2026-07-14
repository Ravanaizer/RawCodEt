#include "mainwindow.h"

void MainWindow::onRemoteLoad(QString filePath) {
  Message msg;
  msg.flag = "load";
  msg.payload["path"] = filePath;
  send(msg);
}

void MainWindow::onRemoteSave(QString filePath) {
  Message msg;
  msg.flag = "save";
  msg.payload["path"] = filePath;
  msg.payload["content"] = getCode();
  send(msg);
}

void MainWindow::remoteGetDirs(QString path) {
  Message msg;
  msg.flag = "ls";
  msg.payload["path"] = path;
  send(msg);
}

void MainWindow::send(const Message &msg) {
  if (m_sock->state() != QAbstractSocket::ConnectedState) {
    QMessageBox::warning(this, "Error", "Not connect to server");
    return;
  }
  m_sock->write(msg.serialize());
}

void MainWindow::onReadyRead() {
  m_buffer.append(m_sock->readAll());

  Message msg;
  while (Message::tryDeserialize(m_buffer, msg)) {
    if (msg.flag == "load") {
      if (msg.payload["ok"].toBool()) {
        QString remotePath = msg.payload["path"].toString();
        QString fileName = QFileInfo(remotePath).fileName();

        // if (fileName.isEmpty() || fileName == "." || fileName == ".." ||
        // fileName.contains("/")) {
        //     m_console->append("Invalid file name from server");
        //     return;
        // }

        QString safePath = m_currentPath + "/" + fileName;

        loadFile(safePath, msg.payload["content"].toString());
      }
    } else if (msg.flag == "save") {
      m_console->append("File saved: " + msg.payload["path"].toString());
    } else if (msg.flag == "ls") {
      m_console->append(msg.payload["content"].toString());
    } else if (msg.flag == "error") {
      QMessageBox::critical(this, "Server fail",
                            msg.payload["message"].toString());
    }
  }
}

void MainWindow::onCommandEntered() {
  QString cmd = m_commandEdit->text();
  if (cmd.trimmed().isEmpty())
    return;
  m_commandEdit->clear();

  // network command with /
  if (cmd.startsWith("/")) {
    QString netCmd = cmd.mid(1).trimmed();
    handleNetworkCommand(netCmd);
    return;
  }

  if (m_shellProcess && m_shellProcess->state() == QProcess::Running) {
    m_console->insertPlainText("$ " + cmd + "\n");
    m_shellProcess->write(cmd.toUtf8() + "\n");
  } else {
    m_console->append("[Shell not running]");
  }
}

void MainWindow::handleNetworkCommand(const QString &cmd) {
  m_console->append("/" + cmd.toHtmlEscaped());

  QStringList parts = cmd.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  if (parts.isEmpty())
    return;
  QString command = parts[0].toLower();
  Message msg;
  bool sendMsg = false;

  if (command == "ls" && parts.size() >= 2) {
    msg.flag = "ls";
    msg.payload["path"] = parts[1];
    sendMsg = true;

  } else if (command == "load" && parts.size() >= 2) {
    msg.flag = "load";
    msg.payload["path"] = parts[1];
    sendMsg = true;

  } else if (command == "save" && parts.size() >= 2) {
    msg.flag = "save";
    msg.payload["path"] = parts[1];
    msg.payload["content"] = getCode();
    sendMsg = true;

  } else if (command == "connect") {
    if (m_sock->state() == QAbstractSocket::ConnectedState) {
      m_console->append("Already connected");
      m_commandEdit->clear();
      return;
    }

    QString host;
    quint16 port;

    if (parts.size() >= 2) {
      QString addr = parts[1];
      if (addr.contains(":")) {
        QStringList addrParts = addr.split(":");
        host = addrParts[0];
        port = addrParts[1].toUShort();
      } else if (parts.size() >= 3) {
        host = parts[1];
        port = parts[2].toUShort();
      } else {
        m_console->append("Format: connect [host:port] or connect host port");
        m_commandEdit->clear();
        return;
      }

      m_hostEdit->setText(host);
      m_portSpin->setValue(port);
    } else {
      host = m_hostEdit->text();
      port = m_portSpin->value();
    }

    m_console->append("Connecting to " + host + ":" + QString::number(port) +
                    "...");
    m_sock->connectToHost(host, port);
    m_commandEdit->clear();
    return;

  } else if (command == "disconnect") {
    m_console->append("Disconnecting...");
    if (m_sock->state() != QAbstractSocket::ConnectedState) {
      m_console->append("Already disconnected");
      m_commandEdit->clear();
      return;
    }
    m_sock->disconnectFromHost();
    m_commandEdit->clear();
    return;

  } else {
    m_console->append("Unknown network command: " + command);
    return;
  }

  if (sendMsg) {
    if (m_sock && m_sock->state() == QAbstractSocket::ConnectedState) {
      m_sock->write(msg.serialize());
    } else {
      m_console->append("Not connected to server");
    }
  }
}
