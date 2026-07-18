#pragma once

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QProcess>
#include <QTcpServer>
#include <QTcpSocket>

#include "message.h"

struct UserInfo {
  QString login;
  QString password;
};

// Remote compilation server
class CompilerServer : public QTcpServer {
  Q_OBJECT

public:
  explicit CompilerServer(QObject *parent = nullptr);

protected:
  // Called when a new client connects
  void incomingConnection(qintptr fd) override;

private slots:
  // Handle incoming data from a specific client socket
  void onReadyRead();

private:
  // --- Command Dispatch ---
  void handle(QTcpSocket *sock, const Message &msg);
  void send(QTcpSocket *sock, const Message &msg);

  // Session management
  QString getUserDir(QTcpSocket *sock) const;
  void createSession(QTcpSocket *sock, const QString &login);
  void destroySession(QTcpSocket *sock);

  // --- Security Layer ---
  // Validate path and check extension if needed
  QString validatePath(QTcpSocket *sock, const QString &path, Message &reply,
                       bool checkExtension = false);

  // Get safe absolute path inside sandbox
  QString getSafePath(const QString &requestPath) const;

  // Check if file extension is allowed
  bool isAllowedExtension(const QString &path) const;

  // Auth
  void loadUsers();
  void handleAuth(QTcpSocket *sock, const Message &msg);
  bool checkCredentials(const QString &login, const QString &password) const;

  // --- Command Handlers ---
  void handleLoad(QTcpSocket *sock, const Message &msg);
  void handleSave(QTcpSocket *sock, const Message &msg);
  void handleLs(QTcpSocket *sock, const Message &msg);
  void handleCompile(QTcpSocket *sock, const Message &msg);
  void handleRun(QTcpSocket *sock, const Message &msg);

  // --- Member Variables ---
  QMap<QTcpSocket *, QByteArray> m_buffers;
  QString m_sandboxDir;

  QMap<QTcpSocket *, QString> m_userDirs;   // sock -> user workspace path
  QMap<QTcpSocket *, QString> m_userLogins; // sock -> login
  QList<UserInfo> m_users;
};
