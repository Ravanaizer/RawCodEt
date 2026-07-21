#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setupUI();
  setupConnects();
}

void MainWindow::setupUI() {
  // --- Main Layout & Splitters ---
  // Horizontal splitter: [File Tree] | [Editor + Console]
  m_mainSplitter = new QSplitter(Qt::Horizontal, this);
  // Vertical splitter for the right side: [Editor] / [Console]
  m_editorSplitter = new QSplitter(Qt::Vertical, m_mainSplitter);

  // --- File Tree (Sidebar) ---
  m_fileTree = new QTreeWidget(m_mainSplitter);
  m_fileTree->setHeaderLabel("Files");
  m_fileTree->setMinimumWidth(200);
  updateFileTree();

  // --- Monaco Editor (WebEngine) ---
  m_EditorSpace = new QWebEngineView(m_editorSplitter);

  // Configure WebEngine settings for local file access and JS
  QWebEngineSettings *settings = m_EditorSpace->settings();
  settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls,
                         true);
  settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls,
                         true);
  settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
  m_EditorSpace->page()->setBackgroundColor(QColor("#1E1F22"));

  // Load the HTML file that initializes Monaco Editor
  QString editorHtmlPath =
      QCoreApplication::applicationDirPath() + "/editor.html";
  m_EditorSpace->setUrl(QUrl::fromLocalFile(editorHtmlPath));
  qDebug() << "Find:" << editorHtmlPath;
  qDebug() << "File expected:" << QFile::exists(editorHtmlPath);

  // Delay setting the "ready" flag to allow Monaco to initialize
  QTimer::singleShot(2000, this, [this]() {
    m_monacoReady = true;
    qDebug() << "Monaco ready";
  });

  // --- Console & Command Input ---
  m_consoleWidget = new QWidget(m_editorSplitter);
  auto *m_consoleLayout = new QVBoxLayout(m_consoleWidget);
  m_consoleLayout->setContentsMargins(0, 0, 0, 0);
  m_consoleLayout->setSpacing(1);

  // Read-only console for output (shell & network)
  m_console = new QTextEdit(m_consoleWidget);
  m_console->setReadOnly(true);
  m_consoleLayout->addWidget(m_console);

  // Command line input
  m_commandEdit = new QLineEdit(m_consoleWidget);
  m_commandEdit->setPlaceholderText(
      "Command: ls/load/save /path, connect/disconnect ip");
  m_consoleLayout->addWidget(m_commandEdit);
  connect(m_commandEdit, &QLineEdit::returnPressed, this,
          &MainWindow::onCommandEntered);

  // --- Network Connection UI (Status Bar) ---
  m_hostEdit = new QLineEdit("127.0.0.1");
  m_portSpin = new QSpinBox;
  m_portSpin->setRange(1, 65535);
  m_portSpin->setValue(5000);

  // Socket initialization
  m_sock = new QTcpSocket(this);

  // Pop-up menu for connection settings
  m_connBtn = new QPushButton("Connect settings");
  m_connBtn->setFlat(true);
  m_connMenu = new QMenu(this);

  // Recreate host/port widgets specifically for the menu layout
  m_hostEdit = new QLineEdit("127.0.0.1");
  m_hostEdit->setMinimumWidth(120);
  m_portSpin = new QSpinBox;
  m_portSpin->setRange(1, 65535);
  m_portSpin->setValue(5000);

  auto *hostLabel = new QLabel("Host:");
  auto *portLabel = new QLabel("Port:");

  auto *connWidget = new QWidget;
  auto *connLayout = new QHBoxLayout(connWidget);
  connLayout->addWidget(hostLabel);
  connLayout->addWidget(m_hostEdit);
  connLayout->addWidget(portLabel);
  connLayout->addWidget(m_portSpin);

  auto *connectAction = new QWidgetAction(m_connMenu);
  connectAction->setDefaultWidget(connWidget);
  m_connMenu->addAction(connectAction);

  // Connect button inside the menu
  m_connectBtnAction = new QAction("Connect", m_connMenu);
  m_connMenu->addAction(m_connectBtnAction);
  m_connBtn->setMenu(m_connMenu);
  statusBar()->addPermanentWidget(m_connBtn);

  // --- Local Shell Process ---
  m_shellProcess = new QProcess;
  m_shellProcess->setProcessChannelMode(
      QProcess::MergedChannels); // Merge stderr and stdout
  m_shellProcess->setWorkingDirectory(m_currentPath);

// Start appropriate shell based on OS
#ifdef Q_OS_WIN
  m_shellProcess->start("powershell", QStringList());
#else
  m_shellProcess->start("bash", QStringList{"--norc", "--noprofile"});
#endif

  // --- Final Window Setup ---
  m_mainSplitter->addWidget(m_fileTree);
  m_mainSplitter->addWidget(m_editorSplitter);
  m_mainSplitter->setStretchFactor(0, 0.5); // Tree takes less space
  m_mainSplitter->setStretchFactor(1, 9.5); // Editor takes most space

  m_editorSplitter->addWidget(m_EditorSpace);
  m_editorSplitter->addWidget(m_consoleWidget);
  m_editorSplitter->setStretchFactor(0, 8);
  m_editorSplitter->setStretchFactor(1, 0.1); // Console is small by default

  // --- Menu Bar & Shortcuts ---
  QMenu *fileMenu = menuBar()->addMenu("File");

  // Open
  QAction *openFileAction = fileMenu->addAction("Open");
  openFileAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
  connect(openFileAction, &QAction::triggered, this, &MainWindow::onOpenFile);

  // Save
  QAction *saveFileAction = fileMenu->addAction("Save");
  saveFileAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
  connect(saveFileAction, &QAction::triggered, this,
          &MainWindow::saveCurrentCode);

  // Save as
  QAction *saveFileActionAs = fileMenu->addAction("Save as");
  saveFileActionAs->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
  connect(saveFileActionAs, &QAction::triggered, this, &MainWindow::saveCodeAs);

  // --- Network Menu ---
  QMenu *networkMenu = menuBar()->addMenu("Network");

  // Connect
  m_connectAction = networkMenu->addAction("Connect");
  m_connectAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));

  // Disconnect
  m_disconnectAction = networkMenu->addAction("Disconnect");
  m_disconnectAction->setEnabled(false);

  networkMenu->addSeparator();

  // Load from server
  m_remoteLoadAction = networkMenu->addAction("Load from Server...");
  m_remoteLoadAction->setShortcut(
      QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));
  m_remoteLoadAction->setEnabled(false);

  // Save to server
  m_remoteSaveAction = networkMenu->addAction("Save to Server...");
  m_remoteSaveAction->setShortcut(
      QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
  m_remoteSaveAction->setEnabled(false);

  // List remote directory
  m_remoteListAction = networkMenu->addAction("List Remote Directory...");
  m_remoteListAction->setEnabled(false);

  networkMenu->addSeparator();

  // Compile
  m_compileAction = networkMenu->addAction("Compile...");
  m_compileAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
  m_compileAction->setEnabled(false);

  // Run
  m_runAction = networkMenu->addAction("Run...");
  m_runAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_F5));
  m_runAction->setEnabled(false);

  setCentralWidget(m_mainSplitter);
  resize(1920, 1080);
  setWindowTitle("RawCodEt - Monaco Editor");
}

void MainWindow::setupConnects() {
  // --- File Tree Interactions ---
  connect(m_fileTree, &QTreeWidget::itemDoubleClicked, this,
          [this](QTreeWidgetItem *item, int) {
            QString path = item->data(0, Qt::UserRole).toString();
            QFileInfo fileInfo(path);

            // Handle "Go Up" directory logic
            if (path == "UP_DIRECTORY") {
              QDir dir(m_currentPath);
              if (dir.cdUp()) {
                m_currentPath = dir.absolutePath();
                updateFileTree();
              }
              return;
            }

            // Open file or navigate into directory
            if (fileInfo.isFile()) {
              loadLocalFile(path);
            } else if (fileInfo.isDir()) {
              m_currentPath = path;
              updateFileTree();
            }
          });

  // --- Monaco Editor "Dirty" State Tracking ---
  // We use the HTML document title to pass state changes from JS to C++
  connect(m_EditorSpace->page(), &QWebEnginePage::titleChanged, this,
          [this](const QString &title) {
            if (!m_monacoReady)
              return;

            if (title.startsWith("MODIFIED:")) {
              if (!m_codeModifiedFlag) {
                m_codeModifiedFlag = true;
                updateWindowTitle();
              }
            } else if (title.startsWith("CLEAN:")) {
              if (m_codeModifiedFlag) {
                m_codeModifiedFlag = false;
                updateWindowTitle();
              }
            }
          });

  // Connect button clicked in the status bar menu
  connect(m_connectBtnAction, &QAction::triggered, this, [this]() {
    QString host = m_hostEdit->text();
    quint16 port = m_portSpin->value();
    m_console->append("Connecting to " + host + ":" + QString::number(port) +
                      "...");
    m_sock->connectToHost(host, port);
  });

  // --- Local Shell Output ---
  connect(m_shellProcess, &QProcess::readyReadStandardOutput, this, [this]() {
    m_console->insertPlainText(
        QString::fromUtf8(m_shellProcess->readAllStandardOutput()));
    // Auto-scroll to bottom
    QTextCursor c = m_console->textCursor();
    c.movePosition(QTextCursor::End);
    m_console->setTextCursor(c);
  });

  // Handle shell termination
  connect(m_shellProcess,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
          [this](int, QProcess::ExitStatus) {
            m_console->append("\n[Shell terminated]");
          });

  // --- Network Menu Connections ---
  // Connect
  connect(m_connectAction, &QAction::triggered, this, [this]() {
    QString host = m_hostEdit->text();
    quint16 port = m_portSpin->value();
    m_console->append("Connecting to " + host + ":" + QString::number(port) +
                      "...");
    m_sock->connectToHost(host, port);
  });

  // Disconnect
  connect(m_disconnectAction, &QAction::triggered, this, [this]() {
    m_console->append("Disconnecting...");
    if (m_sock->state() != QAbstractSocket::ConnectedState) {
      m_console->append("Already disconnected");
      return;
    }
    m_sock->disconnectFromHost();
  });

  // Load from server
  connect(m_remoteLoadAction, &QAction::triggered, this, [this]() {
    bool ok;
    QString path = QInputDialog::getText(
        this, "Remote Load", "Enter file path on server:", QLineEdit::Normal,
        "./test.cpp", &ok);
    if (ok && !path.isEmpty()) {
      onRemoteLoad(path);
    }
  });

  // Save to server
  connect(m_remoteSaveAction, &QAction::triggered, this, [this]() {
    bool ok;
    QString path = QInputDialog::getText(
        this, "Remote Save", "Enter file path on server:", QLineEdit::Normal,
        "./test.cpp", &ok);
    if (ok && !path.isEmpty()) {
      onRemoteSave(path);
    }
  });

  // List remote directory
  connect(m_remoteListAction, &QAction::triggered, this, [this]() {
    bool ok;
    QString path = QInputDialog::getText(
        this, "Remote List", "Enter directory path:", QLineEdit::Normal, ".",
        &ok);
    if (ok && !path.isEmpty()) {
      remoteGetDirs(path);
    }
  });

  // Compile
  connect(m_compileAction, &QAction::triggered, this, [this]() {
    bool ok;
    QString path = QInputDialog::getText(
        this, "Compile", "Enter source file path:", QLineEdit::Normal,
        "./test.cpp", &ok);
    if (ok && !path.isEmpty()) {
      Message msg;
      msg.flag = "compile";
      msg.payload["path"] = path;
      msg.payload["code"] = getCode();
      send(msg);
    }
  });

  // Run
  connect(m_runAction, &QAction::triggered, this, [this]() {
    bool ok;
    QString path = QInputDialog::getText(
        this, "Run", "Enter binary path:", QLineEdit::Normal, "./test.cpp.out",
        &ok);
    if (ok && !path.isEmpty()) {
      Message msg;
      msg.flag = "run";
      msg.payload["path"] = path;
      send(msg);
    }
  });

  // --- Network Socket Events ---
  connect(m_sock, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
  connect(m_sock, &QTcpSocket::connected, this, [this]() {
    m_console->append("Connected");
    m_connectAction->setEnabled(false);
    m_disconnectAction->setEnabled(true);
    m_remoteLoadAction->setEnabled(true);
    m_remoteSaveAction->setEnabled(true);
    m_remoteListAction->setEnabled(true);
    m_compileAction->setEnabled(true);
    m_runAction->setEnabled(true);
  });
  connect(m_sock, &QTcpSocket::disconnected, this, [this]() {
    m_console->append("Disconnected");
    m_connectAction->setEnabled(true);
    m_disconnectAction->setEnabled(false);
    m_remoteLoadAction->setEnabled(false);
    m_remoteSaveAction->setEnabled(false);
    m_remoteListAction->setEnabled(false);
    m_compileAction->setEnabled(false);
    m_runAction->setEnabled(false);
  });
}

void MainWindow::updateFileTree() {
  m_fileTree->clear();
  if (m_currentPath.isEmpty()) {
    m_currentPath = QDir::currentPath();
  }
  loadDirectoryForFileTree(m_currentPath, nullptr);
}

void MainWindow::loadDirectoryForFileTree(const QString &path,
                                          QTreeWidgetItem *parent) {
  QDir dir(path);

  // Add ".." item to navigate up, but only if we are not at the root/home
  if (parent == nullptr && m_currentPath != QDir::homePath() &&
      m_currentPath.startsWith(QDir::homePath())) {
    QTreeWidgetItem *upItem = new QTreeWidgetItem(m_fileTree);
    upItem->setText(0, "..");
    upItem->setData(0, Qt::UserRole, "UP_DIRECTORY");
  }

  // Load directories recursively (skip hidden and build folders)
  QFileInfoList dirList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
  for (const QFileInfo &dirInfo : dirList) {
    if (dirInfo.fileName().startsWith(".") || dirInfo.fileName() == "build")
      continue;

    QTreeWidgetItem *dirItem =
        parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(m_fileTree);
    dirItem->setText(0, dirInfo.fileName() + "/");
    dirItem->setData(0, Qt::UserRole, dirInfo.absoluteFilePath());

    // Recursive call
    loadDirectoryForFileTree(dirInfo.absoluteFilePath(), dirItem);
  }

  // Load files
  QFileInfoList fileList = dir.entryInfoList(QDir::Files);
  for (const QFileInfo &fileInfo : fileList) {
    QTreeWidgetItem *item =
        parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(m_fileTree);
    item->setText(0, fileInfo.fileName());
    item->setData(0, Qt::UserRole, fileInfo.absoluteFilePath());
  }
}

void MainWindow::updateWindowTitle() {
  QString title = "RawCodEt";
  if (!m_currentFilePath.isEmpty()) {
    title += " - " + QFileInfo(m_currentFilePath).fileName();
  }
  // Append asterisk if there are unsaved changes
  if (m_codeModifiedFlag) {
    title += " *";
  }
  setWindowTitle(title);
}

void MainWindow::runJS(const QString &js,
                       std::function<void(const QVariant &)> callback) {
  if (callback) {
    // Asynchronous execution with a callback
    m_EditorSpace->page()->runJavaScript(
        js, [callback](const QVariant &result) { callback(result); });
  } else {
    // Fire-and-forget execution
    m_EditorSpace->page()->runJavaScript(js);
  }
}

void MainWindow::closeEvent(QCloseEvent *event) {
  // Gracefully terminate the local shell process
  if (m_shellProcess && m_shellProcess->state() == QProcess::Running) {
    m_shellProcess->terminate();
    m_shellProcess->waitForFinished(1000);
  }

  // Prompt user if there are unsaved changes in the editor
  if (m_codeModifiedFlag) {
    auto result = QMessageBox::question(
        this, "Unsaved changes",
        "File has unsaved changes. Save before closing?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (result == QMessageBox::Save) {
      saveCurrentCode();
      event->accept();
    } else if (result == QMessageBox::Discard) {
      event->accept();
    } else {
      event->ignore(); // Cancel closing
    }
  } else {
    event->accept();
  }
}

MainWindow::~MainWindow() {
  // Cleanup is handled by Qt's parent-child memory management
}
