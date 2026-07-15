#include "compilerserver.h"

// --- Constructor ---
// Initialize sandbox directory next to the server executable
CompilerServer::CompilerServer(QObject *parent) : QTcpServer(parent) {
  m_sandboxDir = QCoreApplication::applicationDirPath() + "/workspace";
  QDir().mkpath(m_sandboxDir);
  qDebug() << "[SECURITY] Workspace initialized at:" << m_sandboxDir;
}

// --- Connection Lifecycle ---
// Handle new client connection
void CompilerServer::incomingConnection(qintptr fd) {
  auto *sock = new QTcpSocket(this);
  sock->setSocketDescriptor(fd);
  m_buffers[sock] = QByteArray();

  connect(sock, &QTcpSocket::readyRead, this, &CompilerServer::onReadyRead);
  connect(sock, &QTcpSocket::disconnected, this, [this, sock]() {
    m_buffers.remove(sock);
    sock->deleteLater();
  });

  qDebug() << "[SERVER] Client connected from" << sock->peerAddress();
}

// Read incoming data and process messages
void CompilerServer::onReadyRead() {
  auto *sock = qobject_cast<QTcpSocket *>(sender());
  if (!sock)
    return;

  // Append incoming bytes to the per-client buffer
  m_buffers[sock].append(sock->readAll());

  // Process all fully received messages in the buffer
  Message msg;
  while (Message::tryDeserialize(m_buffers[sock], msg)) {
    handle(sock, msg);
  }
}

// --- Universal Path Validation ---
// Validate path and check extension if needed
QString CompilerServer::validatePath(QTcpSocket *sock, const QString &path,
                                     Message &reply, bool checkExtension) {
  // Reject empty paths
  if (path.isEmpty()) {
    reply.flag = "error";
    reply.payload["message"] = "Access denied: empty path";
    send(sock, reply);
    return QString();
  }

  // Check file extension if required
  if (checkExtension && !isAllowedExtension(path)) {
    reply.flag = "error";
    reply.payload["message"] = "Access denied: file extension not allowed";
    send(sock, reply);
    return QString();
  }

  // Validate sandbox via canonical path
  QString safePath = getSafePath(path);
  if (safePath.isEmpty()) {
    reply.flag = "error";
    reply.payload["message"] = "Access denied: path outside workspace";
    send(sock, reply);
    return QString();
  }

  return safePath;
}

// --- Internal Security Methods ---
// Get safe absolute path inside sandbox
QString CompilerServer::getSafePath(const QString &requestPath) const {
  // Clean the path (normalize slashes, collapse dots)
  QString cleanPath = QDir::cleanPath(requestPath);

  // Make absolute path inside the workspace
  QString absolutePath = QDir(m_sandboxDir).absoluteFilePath(cleanPath);

  // Get canonical workspace path (resolves symlinks)
  QString canonicalWorkspace = QDir(m_sandboxDir).canonicalPath();

  // For the file, get canonical path of its parent directory
  QFileInfo fi(absolutePath);
  QString parentDir = fi.absolutePath();
  QString canonicalParent = QDir(parentDir).canonicalPath();

  // If canonical parent is empty or ".", fall back to absolute parent path
  if (canonicalParent.isEmpty() || canonicalParent == ".") {
    canonicalParent = parentDir;
  }

  // CRITICAL CHECK: canonical parent MUST start with canonical workspace
  if (!canonicalParent.startsWith(canonicalWorkspace)) {
    qWarning() << "[SECURITY] Path traversal blocked:" << requestPath
               << "-> resolved to:" << canonicalParent;
    return QString();
  }

  return absolutePath;
}

// Check if file extension is allowed
bool CompilerServer::isAllowedExtension(const QString &path) const {
  // Whitelist of safe file extensions
  static const QStringList allowed = {"cpp",  "c",   "h",   "hpp",  "py", "js",
                                      "html", "css", "txt", "json", "out"};
  QString ext = QFileInfo(path).suffix().toLower();
  return allowed.contains(ext);
}

// --- Command Dispatcher ---

// Dispatch command based on flag
void CompilerServer::handle(QTcpSocket *sock, const Message &msg) {
  qDebug() << "[SERVER] Got flag:" << msg.flag;

  if (msg.flag == "load")
    handleLoad(sock, msg);
  else if (msg.flag == "save")
    handleSave(sock, msg);
  else if (msg.flag == "ls")
    handleLs(sock, msg);
  else if (msg.flag == "compile")
    handleCompile(sock, msg);
  else if (msg.flag == "run")
    handleRun(sock, msg);
  else {
    Message reply;
    reply.flag = "error";
    reply.payload["message"] = "Unknown flag: " + msg.flag;
    send(sock, reply);
  }
}

// --- Command Handlers ---
// Handle load command
void CompilerServer::handleLoad(QTcpSocket *sock, const Message &msg) {
  Message reply;
  QString path = msg.payload["path"].toString();

  // Don't check extension — client can read any file in workspace
  QString safePath = validatePath(sock, path, reply, false);
  if (safePath.isEmpty())
    return;

  QFile f(safePath);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    reply.flag = "error";
    reply.payload["message"] = "Cannot read file: " + f.errorString();
    send(sock, reply);
    return;
  }

  reply.flag = "load";
  reply.payload["ok"] = true;
  reply.payload["content"] = QString::fromUtf8(f.readAll());
  reply.payload["path"] = path;
  send(sock, reply);
}

// Handle save command
void CompilerServer::handleSave(QTcpSocket *sock, const Message &msg) {
  Message reply;
  QString path = msg.payload["path"].toString();
  QString content = msg.payload["content"].toString();

  // Check extension — prevent writing .sh, .exe, etc.
  QString safePath = validatePath(sock, path, reply, true);
  if (safePath.isEmpty())
    return;

  // File size limit (protection against file bombs)
  if (content.length() > 5 * 1024 * 1024) {
    reply.flag = "error";
    reply.payload["message"] = "File too large (max 5MB)";
    send(sock, reply);
    return;
  }

  QFile f(safePath);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
    reply.flag = "error";
    reply.payload["message"] = "Cannot write file: " + f.errorString();
    send(sock, reply);
    return;
  }

  f.write(content.toUtf8());

  reply.flag = "save";
  reply.payload["ok"] = true;
  reply.payload["path"] = path;
  send(sock, reply);
}

// Handle ls command
void CompilerServer::handleLs(QTcpSocket *sock, const Message &msg) {
  Message reply;
  QString path = msg.payload["path"].toString();

  // Don't check extension for directory listing
  QString safePath = validatePath(sock, path, reply, false);
  if (safePath.isEmpty())
    return;

  QDir directory(safePath);
  if (!directory.exists()) {
    reply.flag = "error";
    reply.payload["message"] = "Directory does not exist";
    send(sock, reply);
    return;
  }

  QFileInfoList dirList =
      directory.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
  QStringList names;
  for (const QFileInfo &fi : dirList) {
    QString name = fi.fileName();
    if (fi.isDir())
      name += "/";
    names << name;
  }

  reply.flag = "ls";
  reply.payload["ok"] = true;
  reply.payload["content"] = names.join("\n");
  send(sock, reply);
}

// Handle compile command
void CompilerServer::handleCompile(QTcpSocket *sock, const Message &msg) {
  Message reply;
  QString reqPath = msg.payload["path"].toString();
  QString code = msg.payload["code"].toString();

  // Check extension — only compile allowed languages
  QString safePath = validatePath(sock, reqPath, reply, true);
  if (safePath.isEmpty())
    return;

  // Save source code to disk before compilation
  QFile f(safePath);
  if (!f.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
    reply.flag = "error";
    reply.payload["message"] = "Cannot write source file";
    send(sock, reply);
    return;
  }
  f.write(code.toUtf8());
  f.close();

  // Select compiler based on file extension
  QString ext = QFileInfo(reqPath).suffix().toLower();
  QString program;
  QStringList args;
  QString outBinary = safePath + ".out";

  if (ext == "cpp" || ext == "c") {
    program = "g++";
    args << "-Wall" << "-Wextra" << "-O2" << "-o" << outBinary << safePath;
  } else if (ext == "py") {
    program = "python3";
    args << "-m" << "py_compile" << safePath;
  } else {
    reply.flag = "error";
    reply.payload["message"] = "Compilation not supported for this language";
    send(sock, reply);
    return;
  }

  // Run compiler in an isolated process
  QProcess proc;
  proc.setProcessChannelMode(QProcess::MergedChannels);
  proc.setWorkingDirectory(m_sandboxDir);
  proc.start(program, args);

  if (!proc.waitForStarted(5000)) {
    reply.flag = "error";
    reply.payload["message"] = "Failed to start compiler: " + program;
    send(sock, reply);
    return;
  }

  // Compilation timeout: 30 seconds (prevents infinite loops)
  if (!proc.waitForFinished(30000)) {
    proc.kill();
    reply.flag = "error";
    reply.payload["message"] = "Compilation timeout (30s)";
    send(sock, reply);
    return;
  }

  QString output = QString::fromUtf8(proc.readAllStandardOutput());
  int exitCode = proc.exitCode();

  reply.flag = "compile_result";
  reply.payload["ok"] = (exitCode == 0);
  reply.payload["exit_code"] = exitCode;
  reply.payload["output"] = output;
  reply.payload["path"] = reqPath;

  if (exitCode == 0 && ext != "py") {
    reply.payload["binary"] = reqPath + ".out";
  }

  send(sock, reply);
}

// Handle run command
void CompilerServer::handleRun(QTcpSocket *sock, const Message &msg) {
  Message reply;
  QString reqPath = msg.payload["path"].toString();

  // Don't check extension — binary .out is already in the whitelist
  QString safePath = validatePath(sock, reqPath, reply, false);
  if (safePath.isEmpty())
    return;

  QFileInfo fi(safePath);
  if (!fi.exists()) {
    reply.flag = "error";
    reply.payload["message"] = "Binary not found";
    send(sock, reply);
    return;
  }

  // Execute the binary in an isolated process
  QProcess proc;
  proc.setProcessChannelMode(QProcess::MergedChannels);
  proc.setWorkingDirectory(m_sandboxDir);
  proc.start(safePath);

  if (!proc.waitForStarted(3000)) {
    reply.flag = "error";
    reply.payload["message"] = "Failed to start binary";
    send(sock, reply);
    return;
  }

  // Execution timeout: 10 seconds (prevents infinite loops in user code)
  if (!proc.waitForFinished(10000)) {
    proc.kill();
    reply.flag = "error";
    reply.payload["message"] = "Execution timeout (10s)";
    send(sock, reply);
    return;
  }

  QString output = QString::fromUtf8(proc.readAllStandardOutput());

  reply.flag = "run_result";
  reply.payload["ok"] = (proc.exitCode() == 0);
  reply.payload["exit_code"] = proc.exitCode();
  reply.payload["output"] = output;
  send(sock, reply);
}

// --- Network Helpers ---
// Send message to client
void CompilerServer::send(QTcpSocket *sock, const Message &msg) {
  sock->write(msg.serialize());
}
