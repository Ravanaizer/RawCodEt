#include "compilerserver.h"

// --- Constructor ---
// Initialize sandbox directory next to the server executable
CompilerServer::CompilerServer(QObject *parent)
    : QTcpServer(parent) {
  m_sandboxDir = QCoreApplication::applicationDirPath() + "/workspace";
  QDir().mkpath(m_sandboxDir);
  loadUsers();
  qDebug() << "[AUTH] Workspace:" << m_sandboxDir;
  qDebug() << "[AUTH] Loaded" << m_users.size() << "users";
}

// -- Authentification --
// User database
void CompilerServer::loadUsers() {
  QString usersFile = QCoreApplication::applicationDirPath() + "/users.json";
  QFile f(usersFile);
  if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "[AUTH] Cannot open users.json, using empty list";
    return;
  }

  QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
  QJsonObject root = doc.object();
  QJsonArray usersArray = root["users"].toArray();

  for (const QJsonValue &val : usersArray) {
    QJsonObject obj = val.toObject();
    UserInfo user;
    user.login = obj["login"].toString();
    user.password = obj["password"].toString();
    if (!user.login.isEmpty() && !user.password.isEmpty()) {
      m_users.append(user);
    }
  }
}

bool CompilerServer::checkCredentials(const QString &login,
                                      const QString &password) const {
  for (const UserInfo &user : m_users) {
    if (user.login == login && user.password == password) {
      return true;
    }
  }
  return false;
}

// Session mahager
void CompilerServer::createSession(QTcpSocket *sock, const QString &login) {
  // Sanitize login: only alphanumeric + underscore
  QString safeLogin;
  for (const QChar &c : login) {
    if (c.isLetterOrNumber() || c == '_') {
      safeLogin += c;
    }
  }
  if (safeLogin.isEmpty())
    safeLogin = "unknown";

  QString userDir = QDir(m_sandboxDir).absoluteFilePath(safeLogin);
  QDir().mkpath(userDir);
  m_userDirs[sock] = userDir;
  m_userLogins[sock] = safeLogin;
  qDebug() << "[AUTH] Session created for" << safeLogin << "at" << userDir;
}

QString CompilerServer::getUserDir(QTcpSocket *sock) const {
  auto it = m_userDirs.find(sock);
  return (it != m_userDirs.end()) ? it.value() : QString();
}

void CompilerServer::destroySession(QTcpSocket *sock) {
  QString login = m_userLogins.value(sock);
  QString dir = m_userDirs.value(sock);
  if (!dir.isEmpty()) {
    // Optional: remove user folder
    // QDir(dir).removeRecursively();
    qDebug() << "[AUTH] Session destroyed for" << login;
  }
  m_userDirs.remove(sock);
  m_userLogins.remove(sock);
}

// Auth handler
void CompilerServer::handleAuth(QTcpSocket *sock, const Message &msg) {
  QString login = msg.payload["login"].toString().trimmed();
  QString password = msg.payload["password"].toString();

  Message reply;
  if (login.isEmpty() || password.isEmpty()) {
    reply.flag = "auth_result";
    reply.payload["ok"] = false;
    reply.payload["message"] = "Empty login or password";
    send(sock, reply);
    return;
  }

  if (checkCredentials(login, password)) {
    createSession(sock, login);
    reply.flag = "auth_result";
    reply.payload["ok"] = true;
    reply.payload["message"] = "Welcome, " + login + "!";
    reply.payload["login"] = login;
    send(sock, reply);
  } else {
    reply.flag = "auth_result";
    reply.payload["ok"] = false;
    reply.payload["message"] = "Invalid credentials";
    send(sock, reply);
    // Or disconnect now:
    // sock->disconnectFromHost();
  }
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
    if (m_userDirs.contains(sock)) {
      handle(sock, msg);
    } else {
      if (msg.flag == "auth") {
        handleAuth(sock, msg);
      } else {
        Message reply;
        reply.flag = "error";
        reply.payload["message"] = "Not authenticated. Send 'auth' first.";
        send(sock, reply);
      }
    }
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
  // 1. Clean the path
  QString cleanPath = QDir::cleanPath(requestPath);

  // 2. Special case: "." or empty path → return sandbox directory itself
  if (cleanPath == "." || cleanPath.isEmpty()) {
    return m_sandboxDir;
  }

  // 3. Remove any ".." sequences (paranoid check)
  cleanPath.remove("..");
  cleanPath = QDir::cleanPath(cleanPath);

  // 4. Make absolute path inside sandbox
  QString absolutePath = QDir(m_sandboxDir).absoluteFilePath(cleanPath);

  // 5. Get canonical sandbox path
  QString canonicalWorkspace = QDir(m_sandboxDir).canonicalPath();

  // 6. For the file, get canonical path of its PARENT directory
  QFileInfo fi(absolutePath);
  QString parentDir = fi.absolutePath();
  QString canonicalParent = QDir(parentDir).canonicalPath();

  // 7. Fallback if canonical path is empty or "."
  if (canonicalParent.isEmpty() || canonicalParent == ".") {
    canonicalParent = parentDir;
  }

  // 8. CRITICAL CHECK: canonical parent MUST start with canonical workspace
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

  if (path.isEmpty() || path == "." || path == "./") {
    path = ".";
  }

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
  QString command = msg.payload["command"].toString();

  if (command.isEmpty()) {
    reply.flag = "error";
    reply.payload["message"] = "Empty compile command";
    send(sock, reply);
    return;
  }

  // Parse command
  QStringList parts =
      command.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  if (parts.isEmpty()) {
    reply.flag = "error";
    reply.payload["message"] = "Invalid compile command";
    send(sock, reply);
    return;
  }

  QString compiler = parts[0];

  // Whitelist extension
  static const QStringList allowedCompilers = {
      "g++",    "gcc",   "clang++", "clang", "python3",
      "python", "cmake", "make",    "qmake", "qmake6"};

  if (!allowedCompilers.contains(compiler)) {
    reply.flag = "error";
    reply.payload["message"] = "Compiler not allowed: " + compiler;
    send(sock, reply);
    return;
  }

  // Find -o, -I & imput files
  QString outputFile;
  QStringList inputFiles;
  QStringList compilerArgs;
  QStringList includePaths;

  for (int i = 1; i < parts.size(); ++i) {
    if (parts[i] == "-o" && i + 1 < parts.size()) {
      outputFile = parts[i + 1];
      i++;
    } else if ((parts[i] == "-I" || parts[i] == "-i") && i + 1 < parts.size()) {
      // -I (or -i) for include paths
      includePaths << parts[i + 1];
      i++;
    } else if (parts[i].endsWith(".cpp") || parts[i].endsWith(".c") ||
               parts[i].endsWith(".py") || parts[i].endsWith(".h") ||
               parts[i].endsWith(".hpp")) {
      inputFiles << parts[i];
    } else {
      compilerArgs << parts[i];
    }
  }

  // If not -i, find files in directory
  if (inputFiles.isEmpty()) {
    QDir dir(m_sandboxDir);
    QFileInfoList files = dir.entryInfoList(QDir::Files);
    for (const QFileInfo &fi : files) {
      if (fi.suffix() == "cpp" || fi.suffix() == "c" || fi.suffix() == "py") {
        inputFiles << fi.fileName();
        break;
      }
    }
  }

  if (inputFiles.isEmpty()) {
    reply.flag = "error";
    reply.payload["message"] = "No input files specified or found";
    send(sock, reply);
    return;
  }

  // If output not found, auto create
  if (outputFile.isEmpty()) {
    QString baseName = QFileInfo(inputFiles[0]).baseName();
    outputFile = baseName + ".out";
  }

  // Resolve path to sandbox
  QString safeOutput = validatePath(sock, outputFile, reply, false);
  if (safeOutput.isEmpty())
    return;

  QStringList safeInputs;
  for (const QString &input : inputFiles) {
    QString safeInput = validatePath(sock, input, reply, false);
    if (safeInput.isEmpty())
      return;
    safeInputs << safeInput;
  }

  // Resolve include paths
  QStringList safeIncludePaths;
  for (const QString &incPath : includePaths) {
    QString safeIncPath = validatePath(sock, incPath, reply, false);
    if (safeIncPath.isEmpty())
      return;
    safeIncludePaths << safeIncPath;
  }

  // Create finel command
  QStringList finalArgs;
  finalArgs << compilerArgs;

  // Add include paths with -I
  for (const QString &safeInc : safeIncludePaths) {
    finalArgs << "-I" << safeInc;
  }

  if (compiler == "g++" || compiler == "gcc" || compiler == "clang++" ||
      compiler == "clang") {
    finalArgs << "-o" << safeOutput << safeInputs;
  } else if (compiler == "python3" || compiler == "python") {
    finalArgs << "-m" << "py_compile" << safeInputs;
  } else if (compiler == "cmake") {
    finalArgs << m_sandboxDir;
  }

  // Run compile
  QProcess proc;
  proc.setProcessChannelMode(QProcess::MergedChannels);
  proc.setWorkingDirectory(m_sandboxDir);
  proc.start(compiler, finalArgs);

  if (!proc.waitForStarted(5000)) {
    reply.flag = "error";
    reply.payload["message"] = "Failed to start compiler: " + compiler;
    send(sock, reply);
    return;
  }

  int timeout = (compiler == "cmake" || compiler == "make") ? 60000 : 30000;
  if (!proc.waitForFinished(timeout)) {
    proc.kill();
    reply.flag = "error";
    reply.payload["message"] =
        "Compilation timeout (" + QString::number(timeout / 1000) + "s)";
    send(sock, reply);
    return;
  }

  QString output = QString::fromUtf8(proc.readAllStandardOutput());
  int exitCode = proc.exitCode();

  reply.flag = "compile_result";
  reply.payload["ok"] = (exitCode == 0);
  reply.payload["exit_code"] = exitCode;
  reply.payload["output"] = output;
  reply.payload["command"] = command;

  if (exitCode == 0 && !safeOutput.isEmpty()) {
    reply.payload["binary"] = outputFile;
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
