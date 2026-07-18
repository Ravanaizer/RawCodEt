#include "mainwindow.h"

// --- Remote Network Command Builders ---

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

// Serializes and sends a message over the TCP socket
void MainWindow::send(const Message &msg) {
  if (m_sock->state() != QAbstractSocket::ConnectedState) {
    QMessageBox::warning(this, "Error", "Not connected to server");
    return;
  }
  m_sock->write(msg.serialize());
}

// --- Network Response Handler ---

// Reads incoming data, buffers it, and processes complete messages
void MainWindow::onReadyRead() {
  m_buffer.append(m_sock->readAll());
  Message msg;

  // Process all fully received messages in the buffer
  while (Message::tryDeserialize(m_buffer, msg)) {
    if (msg.flag == "auth_result") {
      if (msg.payload["ok"].toBool()) {
        QString login = msg.payload["login"].toString();
        m_console->append("Authenticated as: " +
                        login.toHtmlEscaped());
      } else {
        m_console->append("Auth failed: " +
                        msg.payload["message"].toString().toHtmlEscaped());
      }
    } else if (msg.flag == "load") {
      if (msg.payload["ok"].toBool()) {
        QString remotePath = msg.payload["path"].toString();
        QString fileName = QFileInfo(remotePath).fileName();

        // Save remote file locally to load it into the editor
        QString safePath = m_currentPath + "/" + fileName;
        loadFile(safePath, msg.payload["content"].toString());
      }
    } else if (msg.flag == "save") {
      m_console->append("File saved: " + msg.payload["path"].toString());
    } else if (msg.flag == "ls") {
      m_console->append(msg.payload["content"].toString());
    } else if (msg.flag == "compile_result") {
      if (msg.payload["ok"].toBool()) {
        m_console->append("Compilation successful");
      } else {
        m_console->append("Compilation failed (exit " +
                          QString::number(msg.payload["exit_code"].toInt()) +
                          ")");
      }

      QString output = msg.payload["output"].toString();
      if (!output.isEmpty()) {
        m_console->append(output.toHtmlEscaped());
      }
    } else if (msg.flag == "run_result") {
      if (msg.payload["ok"].toBool()) {
        m_console->append("Run successful");
      } else {
        m_console->append("Run failed (exit " +
                          QString::number(msg.payload["exit_code"].toInt()) +
                          ")");
      }

      QString output = msg.payload["output"].toString();
      if (!output.isEmpty()) {
        m_console->append(output.toHtmlEscaped());
      }
    } else if (msg.flag == "error") {
      QMessageBox::critical(this, "Server fail",
                            msg.payload["message"].toString());
    }
  }
}

// --- Local Console & Command Routing ---

// Handles input from the command line (local shell or network commands)
void MainWindow::onCommandEntered() {
  QString cmd = m_commandEdit->text();
  if (cmd.trimmed().isEmpty())
    return;

  m_commandEdit->clear();

  // Commands starting with '/' are routed to the network handler
  if (cmd.startsWith("/")) {
    QString netCmd = cmd.mid(1).trimmed();
    handleNetworkCommand(netCmd);
    return;
  }

  // Otherwise, send to the local bash/powershell process
  if (m_shellProcess && m_shellProcess->state() == QProcess::Running) {
    m_console->insertPlainText("$ " + cmd + "\n");
    m_shellProcess->write(cmd.toUtf8() + "\n");
  } else {
    m_console->append("[Shell not running]");
  }
}

// Parses and executes network commands (e.g., /compile, /connect)
void MainWindow::handleNetworkCommand(const QString &cmd) {
  m_console->append("/" + cmd.toHtmlEscaped());
  QStringList parts = cmd.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  if (parts.isEmpty())
    return;

  QString command = parts[0].toLower();
  Message msg;
  bool sendMsg = false;

  // --- File & Compilation Commands ---
  if (command == "auth" && parts.size() >= 3) {
    msg.flag = "auth";
    msg.payload["login"] = parts[1];
    msg.payload["password"] = parts[2];
    sendMsg = true;
  } else if (command == "ls" && parts.size() >= 2) {
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
  } else if (command == "compile" && parts.size() >= 2) {
    msg.flag = "compile";
    QString fullCommand = cmd.mid(8).trimmed();
    msg.payload["command"] = fullCommand;
    sendMsg = true;
  } else if (command == "run" && parts.size() >= 2) {
    msg.flag = "run";
    msg.payload["path"] = parts[1];
    sendMsg = true;
  }

  // --- Connection Management ---
  else if (command == "connect") {
    if (m_sock->state() == QAbstractSocket::ConnectedState) {
      m_console->append("Already connected");
      return;
    }

    QString host;
    quint16 port;

    // Parse connection string (e.g., /connect 127.0.0.1:5000 OR /connect
    // 127.0.0.1 5000)
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
        return;
      }
      m_hostEdit->setText(host);
      m_portSpin->setValue(port);
    } else {
      // Use values from the UI if no arguments provided
      host = m_hostEdit->text();
      port = m_portSpin->value();
    }

    m_console->append("Connecting to " + host + ":" + QString::number(port) +
                      "...");
    m_sock->connectToHost(host, port);
    return;
  } else if (command == "disconnect") {
    m_console->append("Disconnecting...");
    if (m_sock->state() != QAbstractSocket::ConnectedState) {
      m_console->append("Already disconnected");
      return;
    }
    m_sock->disconnectFromHost();
    return;
  } else {
    m_console->append("Unknown network command: " + command);
    return;
  }

  // Send the constructed message if it's a file/network operation
  if (sendMsg) {
    if (m_sock && m_sock->state() == QAbstractSocket::ConnectedState) {
      m_sock->write(msg.serialize());
    } else {
      m_console->append("Not connected to server");
    }
  }
}
