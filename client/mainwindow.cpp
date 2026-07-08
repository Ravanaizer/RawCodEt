#include "mainwindow.h"
// #include "ui_mainwindow.h"
#include <QAction>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonValue>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include <QWidgetAction>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
// , ui(new Ui::MainWindow)
{
  // ui->setupUi(this);

  // Set splitter for tree and editor
  mainSplitter = new QSplitter(Qt::Horizontal, this);
  editorSplitter = new QSplitter(Qt::Vertical, mainSplitter);

  // Add file tree
  fileTree = new QTreeWidget(mainSplitter);
  fileTree->setHeaderLabel("Files");
  fileTree->setMinimumWidth(200);
  updateFileTree();

  // Setup Editor
  EditorSpace = new QWebEngineView(editorSplitter);

  // Settings for editor
  QWebEngineSettings *settings = EditorSpace->settings();
  settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls,
                         true);
  settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls,
                         true);
  settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);

  // Set html for editor
  QString editorHtmlPath =
      QCoreApplication::applicationDirPath() + "/editor.html";
  EditorSpace->setUrl(QUrl::fromLocalFile(editorHtmlPath));
  qDebug() << "Find:" << editorHtmlPath;
  qDebug() << "File expected:" << QFile::exists(editorHtmlPath);

  QTimer::singleShot(2000, this, [this]() {
    monacoReady = true;
    qDebug() << "Monaco ready";
  });

  // Console + input widget
  consoleWidget = new QWidget(editorSplitter);
  auto *consoleLayout = new QVBoxLayout(consoleWidget);
  consoleLayout->setContentsMargins(0, 0, 0, 0);
  consoleLayout->setSpacing(1);

  // Console
  console = new QTextEdit(consoleWidget);
  console->setReadOnly(true);
  // console->setFont(QFont("Monospace", 10));
  // console->setStyleSheet("QTextEdit { background-color: #1e1e1e; color:
  // #d4d4d4; }");
  consoleLayout->addWidget(console);

  // Input
  commandEdit = new QLineEdit(consoleWidget);
  // commandEdit->setFont(QFont("Monospace", 10));
  commandEdit->setPlaceholderText(
      "Command: ls/load/save /path, connect/disconnect ip");
  // commandEdit->setStyleSheet("QLineEdit { padding: 5px; }");
  consoleLayout->addWidget(commandEdit);

  connect(commandEdit, &QLineEdit::returnPressed, this,
          &MainWindow::onCommandEntered);

  hostEdit = new QLineEdit("127.0.0.1");
  portSpin = new QSpinBox;
  portSpin->setRange(1, 65535);
  portSpin->setValue(5000);

  // Window setup
  mainSplitter->addWidget(fileTree);
  mainSplitter->addWidget(editorSplitter);
  mainSplitter->setStretchFactor(0, 0.5);
  mainSplitter->setStretchFactor(1, 9.5);
  editorSplitter->addWidget(EditorSpace);
  editorSplitter->addWidget(consoleWidget);
  editorSplitter->setStretchFactor(0, 8);
  editorSplitter->setStretchFactor(1, 0.1);

  // Shortcuts
  QMenu *fileMenu = menuBar()->addMenu("File");
  QAction *openFileAction = fileMenu->addAction("Open");
  openFileAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
  connect(openFileAction, &QAction::triggered, this, &MainWindow::onOpenFile);
  QAction *saveFileAction = fileMenu->addAction("Save");
  saveFileAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
  connect(saveFileAction, &QAction::triggered, this,
          &MainWindow::saveCurrentCode);
  QAction *saveFileActionAs = fileMenu->addAction("Save as");
  saveFileActionAs->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
  connect(saveFileActionAs, &QAction::triggered, this, &MainWindow::saveCodeAs);

  // Socket
  sock = new QTcpSocket(this);

  // Button to connect
  connBtn = new QPushButton("Connect settings");
  // connBtn->setStyleSheet("QPushButton { color: red; font-weight: bold; }");
  connBtn->setFlat(true);

  // Pop-up menu
  connMenu = new QMenu(this);

  hostEdit = new QLineEdit("127.0.0.1");
  hostEdit->setMinimumWidth(120);
  portSpin = new QSpinBox;
  portSpin->setRange(1, 65535);
  portSpin->setValue(5000);

  auto *hostLabel = new QLabel("Host:");
  auto *portLabel = new QLabel("Port:");

  auto *connWidget = new QWidget;
  auto *connLayout = new QHBoxLayout(connWidget);
  connLayout->addWidget(hostLabel);
  connLayout->addWidget(hostEdit);
  connLayout->addWidget(portLabel);
  connLayout->addWidget(portSpin);

  auto *connectAction = new QWidgetAction(connMenu);
  connectAction->setDefaultWidget(connWidget);
  connMenu->addAction(connectAction);

  auto *connectBtnAction = new QAction("Connect", connMenu);
  connMenu->addAction(connectBtnAction);

  connBtn->setMenu(connMenu);
  statusBar()->addPermanentWidget(connBtn);

  setCentralWidget(mainSplitter);
  resize(1920, 1080); // FHD (1280 x 720 for HD)
  setWindowTitle("RawCodEt - Monaco Editor");

  // File tree double click options
  connect(fileTree, &QTreeWidget::itemDoubleClicked, this,
          [this](QTreeWidgetItem *item, int) {
            QString path = item->data(0, Qt::UserRole).toString();
            QFileInfo fileInfo(path);

            if (path == "UP_DIRECTORY") {
              QDir dir(currentPath);
              if (dir.cdUp()) {
                currentPath = dir.absolutePath();
                updateFileTree();
              }
              return;
            }

            if (fileInfo.isFile()) {
              loadLocalFile(path);
            } else if (fileInfo.isDir()) {
              currentPath = path;
              updateFileTree();
            }
          });

  // Check for code modified
  connect(EditorSpace->page(), &QWebEnginePage::titleChanged, this,
          [this](const QString &title) {
            if (!monacoReady)
              return;

            if (title.startsWith("MODIFIED:")) {
              if (!codeModifiedFlag) {
                codeModifiedFlag = true;
                updateTitle();
              }
            } else if (title.startsWith("CLEAN:")) {
              if (codeModifiedFlag) {
                codeModifiedFlag = false;
                updateTitle();
              }
            }
          });

  // Connect to host
  connect(sock, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
  connect(sock, &QTcpSocket::connected, this,
          [this]() { console->append("Connected"); });
  connect(sock, &QTcpSocket::disconnected, this,
          [this]() { console->append("Disconnected"); });
  connect(connectBtnAction, &QAction::triggered, this, [this]() {
    QString host = hostEdit->text();
    quint16 port = portSpin->value();
    console->append("Connecting to " + host + ":" + QString::number(port) +
                    "...");
    sock->connectToHost(host, port);
  });
}

void MainWindow::updateFileTree() {
  fileTree->clear();

  if (currentPath.isEmpty()) {
    currentPath = QDir::currentPath();
  }

  loadDirectory(currentPath, nullptr);

  // fileTree->expandAll();
}

void MainWindow::loadDirectory(const QString &path, QTreeWidgetItem *parent) {
  QDir dir(path);

  // QStringList filters;
  // filters << "*.cpp" << "*.h" << "*.ui" << "*.pro" << "*.html" << "*.js" <<
  // "*.css";

  if (parent == nullptr && currentPath != QDir::homePath() &&
      currentPath.startsWith(QDir::homePath())) {
    QTreeWidgetItem *upItem = new QTreeWidgetItem(fileTree);
    upItem->setText(0, "..");
    upItem->setData(0, Qt::UserRole, "UP_DIRECTORY");
  }

  QFileInfoList fileList = dir.entryInfoList(QDir::Files);
  // or (filters, QDir::Files)
  for (const QFileInfo &fileInfo : fileList) {
    QTreeWidgetItem *item;
    if (parent) {
      item = new QTreeWidgetItem(parent);
    } else {
      item = new QTreeWidgetItem(fileTree);
    }
    item->setText(0, fileInfo.fileName());
    item->setData(0, Qt::UserRole, fileInfo.absoluteFilePath());
  }

  QFileInfoList dirList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
  for (const QFileInfo &dirInfo : dirList) {
    if (dirInfo.fileName().startsWith(".") || dirInfo.fileName() == "build")
      continue;

    QTreeWidgetItem *dirItem;
    if (parent) {
      dirItem = new QTreeWidgetItem(parent);
    } else {
      dirItem = new QTreeWidgetItem(fileTree);
    }
    dirItem->setText(0, dirInfo.fileName());
    dirItem->setData(0, Qt::UserRole, dirInfo.absoluteFilePath());

    loadDirectory(dirInfo.absoluteFilePath(), dirItem);
  }
}

// Set title text
void MainWindow::updateTitle() {
  QString title = "RawCodEt";
  if (!currentFilePath.isEmpty() && currentFilePath != "") {
    title += " - " + QFileInfo(currentFilePath).fileName();
  }
  if (codeModifiedFlag) {
    title += " *";
  }
  setWindowTitle(title);
}

// Select file for open and load
void MainWindow::onOpenFile() {
  QString filePath = QFileDialog::getOpenFileName(
      this, "Open file", currentPath,
      "Все файлы (*.*);;C++ файлы (*.cpp *.h *.hpp);;Python "
      "(*.py);;JavaScript(*.js);; HTML(*.html)");

  if (!filePath.isEmpty()) {
    loadLocalFile(filePath);
  }

  currentPath = QFileInfo(filePath).absolutePath();
  updateFileTree();
}

void MainWindow::loadLocalFile(const QString &filePath) {
  QFile file(filePath);

  QString base64Content = "";

  if (file.open(QIODevice::ReadOnly)) {
    QByteArray rawData = file.readAll();
    file.close();

    QString base64Content = rawData.toBase64();
  }

  loadFile(filePath, base64Content);
}

// Load code from file
void MainWindow::loadFile(const QString &filePath, const QString &content) {
  QString base64Content = content.toUtf8().toBase64();
  runJS(QString("editor.setValue(atob('%1'));").arg(base64Content));

  QString lang = "cpp";
  if (filePath.endsWith(".py"))
    lang = "python";
  else if (filePath.endsWith(".js"))
    lang = "javascript";
  else if (filePath.endsWith(".html"))
    lang = "html";

  runJS(QString("monaco.editor.setModelLanguage(editor.getModel(),'%1');")
            .arg(lang));
  runJS(QString("window.currentFileName ='%1';")
            .arg(QFileInfo(filePath).fileName()));
  runJS("window.setupChangeHandler();");

  currentFilePath = filePath;
  codeModifiedFlag = false;
  updateTitle();
}

// Get current codee from Editor
QString MainWindow::getCode() {
  QString code;
  QEventLoop loop;
  runJS("btoa(editor.getValue());", [&code, &loop](const QVariant &result) {
    code = QByteArray::fromBase64(result.toString().toUtf8());
    loop.quit();
  });
  loop.exec();
  return code;
}

// Save code to choosed file
void MainWindow::saveCodeAs() {
  QString filePath = QFileDialog::getOpenFileName(
      this, "Open file", currentPath,
      "Все файлы (*.*);;C++ файлы (*.cpp *.h *.hpp);;Python (*.py);;JavaScript "
      "(*.js);; HTML(*.html)");

  saveCode(filePath);
}

// Save code to current file
void MainWindow::saveCurrentCode() { MainWindow::saveCode(currentFilePath); }

// Save code main function
void MainWindow::saveCode(QString filePath) {
  if (!filePath.isEmpty()) {
    QString codeText = getCode();

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      file.write(codeText.toUtf8());
      file.close();

      runJS("window.originalContent = editor.getValue();");
      runJS(QString("document.title = 'CLEAN:%1';")
                .arg(QFileInfo(filePath).fileName()));

      codeModifiedFlag = false;
      updateTitle();
    } else {
      qDebug() << "Could not open file for writing:" << file.errorString();
    }
  }
}

void MainWindow::closeEvent(QCloseEvent *event) {

  if (codeModifiedFlag) {

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
      event->ignore();
    }

  } else {
    event->accept();
  }
}

void MainWindow::runJS(const QString &js,
                       std::function<void(const QVariant &)> callback) {
  if (callback) {
    EditorSpace->page()->runJavaScript(
        js, [callback](const QVariant &result) { callback(result); });
  } else {
    EditorSpace->page()->runJavaScript(js);
  }
}

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
  if (sock->state() != QAbstractSocket::ConnectedState) {
    QMessageBox::warning(this, "Error", "Not connect to server");
    return;
  }
  sock->write(msg.serialize());
}

void MainWindow::onReadyRead() {
  buffer.append(sock->readAll());

  Message msg;
  while (Message::tryDeserialize(buffer, msg)) {
    if (msg.flag == "load") {
      if (msg.payload["ok"].toBool()) {
        QString remotePath = msg.payload["path"].toString();
        QString fileName = QFileInfo(remotePath).fileName();

        // if (fileName.isEmpty() || fileName == "." || fileName == ".." || fileName.contains("/")) {
        //     console->append("Invalid file name from server");
        //     return;
        // }

        QString safePath = currentPath + "/" + fileName;

        loadFile(safePath, msg.payload["content"].toString());
      }
    } else if (msg.flag == "save") {
      console->append("File saved: " + msg.payload["path"].toString());
    } else if (msg.flag == "ls") {
      console->append(msg.payload["content"].toString());
    } else if (msg.flag == "error") {
      QMessageBox::critical(this, "Server fail",
                            msg.payload["message"].toString());
    }
  }
}

void MainWindow::onCommandEntered() {
  QString cmd = commandEdit->text().trimmed();
  if (cmd.isEmpty())
    return;

  console->append(cmd.toHtmlEscaped());

  QStringList parts = cmd.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  QString command = parts[0].toLower();

  Message msg;

  if (command == "ls" && parts.size() >= 2) {
    msg.flag = "ls";
    if (parts.size() >= 2) {
      msg.payload["path"] = parts[1];
    } else {
      msg.payload["path"] = ".";
    }
  } else if (command == "load" && parts.size() >= 2) {
    msg.flag = "load";
    msg.payload["path"] = parts[1];
  } else if (command == "save" && parts.size() >= 2) {
    msg.flag = "save";
    msg.payload["path"] = parts[1];
    msg.payload["content"] = getCode();
    commandEdit->clear();
  } else if (command == "connect") {
    if (sock->state() == QAbstractSocket::ConnectedState) {
      console->append("Already connected");
      commandEdit->clear();
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
        console->append("Format: connect [host:port] or connect host port");
        commandEdit->clear();
        return;
      }

      hostEdit->setText(host);
      portSpin->setValue(port);
    } else {
      host = hostEdit->text();
      port = portSpin->value();
    }

    console->append("Connecting to " + host + ":" + QString::number(port) +
                    "...");
    sock->connectToHost(host, port);
  } else {
    // console->append(command.toHtmlEscaped());
    commandEdit->clear();
    return;
  }

  if (sock && sock->state() == QAbstractSocket::ConnectedState) {
    sock->write(msg.serialize());
  } else {
    console->append("Server fail");
  }

  commandEdit->clear();
}

MainWindow::~MainWindow() {
  // delete ui;
}
