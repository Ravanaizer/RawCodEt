// #include "mainwindow.h"
// // #include "ui_mainwindow.h"
// #include <QWebEngineView>
// #include <QWebEngineSettings>
// #include <QCoreApplication>
// #include <QDir>
// #include <QUrl>
// #include <QFileInfo>
// #include <QDebug>
// #include <QFileDialog>
// #include <QFile>
// #include <QTextStream>
// #include <QJsonDocument>
// #include <QJsonValue>
// #include <QMenuBar>
// #include <QAction>
// #include <QMessageBox>
// #include <QCloseEvent>
// #include <QSplitter>
// #include <QTreeWidgetItem>
// #include <QHeaderView>
// #include <QTimer>


// MainWindow::MainWindow(QWidget *parent)
//     : QMainWindow(parent)
//     // , ui(new Ui::MainWindow)
// {
//     // ui->setupUi(this);

//     // Set splitter for tree and editor
//     QSplitter *viewSplitter = new QSplitter(Qt::Horizontal, this);

//     // Add file tree
//     fileTree = new QTreeWidget(this);
//     fileTree->setHeaderLabel("Files");
//     fileTree->setMinimumWidth(200);
//     updateFileTree();

//     // File tree double click options
//     connect(fileTree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem *item, int){
//       QString path = item->data(0, Qt::UserRole).toString();
//       QFileInfo fileInfo(path);

//       if (path == "UP_DIRECTORY") {
//         QDir dir(currentPath);
//         if (dir.cdUp()) {
//           currentPath = dir.absolutePath();
//           updateFileTree();
//         }
//         return;
//       }

//       if (fileInfo.isFile()) {
//         loadFile(path);
//       }
//       else if (fileInfo.isDir()) {
//         currentPath = path;
//         updateFileTree();
//       }
//     });


//     // Setup Editor
//     EditorSpace = new QWebEngineView(this);

//     // Check for code modified
//     connect(EditorSpace->page(), &QWebEnginePage::titleChanged, this, [this](const QString &title) {
//       if (!monacoReady) return;

//       if (title.startsWith("MODIFIED:")) {
//         if (!codeModifiedFlag) {
//           codeModifiedFlag = true;
//           updateTitle();
//         }
//       }
//       else if (title.startsWith("CLEAN:")) {
//         if (codeModifiedFlag) {
//           codeModifiedFlag = false;
//           updateTitle();
//         }
//       }
//     });

//     // Settings for editor
//     QWebEngineSettings *settings = EditorSpace->settings();
//     settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
//     settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
//     settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);

//     // Set html for editor
//     QString editorHtmlPath = QCoreApplication::applicationDirPath() + "/editor.html";
//     EditorSpace->setUrl(QUrl::fromLocalFile(editorHtmlPath));
//     qDebug() << "Find:" << editorHtmlPath;
//     qDebug() << "File expected:" << QFile::exists(editorHtmlPath);

//     QTimer::singleShot(2000, this, [this]() {
//       monacoReady = true;
//       qDebug() << "Monaco ready";
//     });

//     // Window setup
//     viewSplitter->addWidget(fileTree);
//     viewSplitter->addWidget(EditorSpace);
//     viewSplitter->setStretchFactor(0, 1);
//     viewSplitter->setStretchFactor(1, 7);

//     // Shortcuts
//     QMenu *fileMenu = menuBar()->addMenu("File");
//     QAction *openFileAction = fileMenu->addAction("Open");
//     openFileAction -> setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
//     connect(openFileAction, &QAction::triggered, this, &MainWindow::onOpenFile);
//     QAction *saveFileAction = fileMenu->addAction("Save");
//     saveFileAction -> setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
//     connect(saveFileAction, &QAction::triggered, this, &MainWindow::saveCurrentCode);
//     QAction *saveFileActionAs = fileMenu->addAction("Save as");
//     saveFileActionAs -> setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_S));
//     connect(saveFileActionAs, &QAction::triggered, this, &MainWindow::saveCodeAs);

//     setCentralWidget(viewSplitter);
//     resize(1920, 1080); // FHD (1280 x 720 for HD)
//     setWindowTitle("RawCodEt - Monaco Editor");
// }


// void MainWindow::updateFileTree() {
//   fileTree->clear();

//   if (currentPath.isEmpty()) {
//     currentPath = QDir::currentPath();
//   }

//   loadDirectory(currentPath, nullptr);

//   // fileTree->expandAll();
// }


// void MainWindow::loadDirectory(const QString &path, QTreeWidgetItem *parent) {
//   QDir dir(path);

//   // QStringList filters;
//   // filters << "*.cpp" << "*.h" << "*.ui" << "*.pro" << "*.html" << "*.js" << "*.css";

//   if (parent == nullptr && currentPath != QDir::homePath() && currentPath.startsWith(QDir::homePath())) {  // только для корневых элементов
//     QTreeWidgetItem *upItem = new QTreeWidgetItem(fileTree);
//     upItem->setText(0, "..");
//     upItem->setData(0, Qt::UserRole, "UP_DIRECTORY");
//   }

//   QFileInfoList fileList = dir.entryInfoList(QDir::Files); // or (filters, QDir::Files)
//   for (const QFileInfo &fileInfo : fileList) {
//     QTreeWidgetItem *item;
//     if (parent) {
//       item = new QTreeWidgetItem(parent);
//     } else {
//       item = new QTreeWidgetItem(fileTree);
//     }
//     item->setText(0, fileInfo.fileName());
//     item->setData(0, Qt::UserRole, fileInfo.absoluteFilePath());
//   }


//   QFileInfoList dirList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
//   for (const QFileInfo &dirInfo : dirList) {
//     if (dirInfo.fileName().startsWith(".") || dirInfo.fileName() == "build")
//       continue;

//     QTreeWidgetItem *dirItem;
//     if (parent) {
//       dirItem = new QTreeWidgetItem(parent);
//     } else {
//       dirItem = new QTreeWidgetItem(fileTree);
//     }
//     dirItem->setText(0, dirInfo.fileName());
//     dirItem->setData(0, Qt::UserRole, dirInfo.absoluteFilePath());

//     loadDirectory(dirInfo.absoluteFilePath(), dirItem);
//   }
// }


// // Select file for open and load
// void MainWindow::onOpenFile() {
//   QString filePath = QFileDialog::getOpenFileName(
//     this,
//     "Open file",
//     currentPath,
//     "Все файлы (*.*);;C++ файлы (*.cpp *.h *.hpp);;Python (*.py);;JavaScript (*.js);;HTML (*.html)"
//   );

//   if (!filePath.isEmpty()) {
//     loadFile(filePath);
//   }

//   currentPath = QFileInfo(filePath).absolutePath();
//   updateFileTree();
// }


// // Set title text
// void MainWindow::updateTitle()
// {
//     QString title = "RawCodEt";
//     if (!currentFilePath.isEmpty() && currentFilePath != "") {
//         title += " - " + QFileInfo(currentFilePath).fileName();
//     }
//     if (codeModifiedFlag) {
//         title += " *";
//     }
//     setWindowTitle(title);
// }


// // Load code from file
// void MainWindow::loadFile(const QString &filePath) {
//   QFile file(filePath);

//   if (file.open(QIODevice::ReadOnly)) {
//     // Get byte array from file and close file
//     QByteArray rawData = file.readAll();
//     file.close();

//     // Byte array to UTF-8 string
//     QString content = QString::fromUtf8(rawData);

//     // UTF-8 string to JSON string
//     QJsonValue jsonVal(content);
//     QString jsonStr = QString::fromUtf8(jsonVal.toJson(QJsonDocument::Compact));

//     monacoReady = false;

//     // Set JSON string to editor
//     EditorSpace->page()->runJavaScript(QString("editor.setValue(%1);").arg(jsonStr));

//     // Select syntax highlighting
//     QString lang = "cpp";
//     if (filePath.endsWith(".py")) lang = "python";
//     else if (filePath.endsWith(".js")) lang = "javascript";
//     else if (filePath.endsWith(".html")) lang = "html";

//     EditorSpace->page()->runJavaScript(QString("monaco.editor.setModelLanguage(editor.getModel(), '%1');").arg(lang));

//     EditorSpace->page()->runJavaScript(QString("window.currentFileName = '%1';").arg(QFileInfo(filePath).fileName()));

//     EditorSpace->page()->runJavaScript("window.setupChangeHandler();");

//     currentFilePath = filePath;

//     codeModifiedFlag = false;
//     updateTitle();

//     QTimer::singleShot(300, this, [this]() {
//       monacoReady = true;
//     });
//     // qDebug() << "File is open successfully:" << filePath;
//   // } else {
//   //   // qDebug() << "Error:" << filePath;
//   // }
//   }
// }


// // Get current codee from Editor
// QString MainWindow::getCode()
// {
//     QString code;
//     QEventLoop loop;
//     EditorSpace->page()->runJavaScript("editor.getValue();", [&code, &loop](const QVariant &result) {
//         code = result.toString();
//         loop.quit();
//     });
//     loop.exec();
//     return code;
// }


// // Save code to choosed file
// void MainWindow::saveCodeAs() {
//   QString filePath = QFileDialog::getOpenFileName(
//     this,
//     "Open file",
//     currentPath,
//     "Все файлы (*.*);;C++ файлы (*.cpp *.h *.hpp);;Python (*.py);;JavaScript (*.js);;HTML (*.html)"
//   );

//   saveCode(filePath);
// }


// // Save code to current file
// void MainWindow::saveCurrentCode() {
//   MainWindow::saveCode(currentFilePath);
// }


// // Save code main function
// void MainWindow::saveCode(QString filePath) {
//   if (!filePath.isEmpty()) {
//     QString codeText = getCode();

//     QFile file(filePath);
//     if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
//       file.write(codeText.toUtf8());
//       file.close();

//       EditorSpace->page()->runJavaScript("window.originalContent = editor.getValue();");
//       EditorSpace->page()->runJavaScript(
//         QString("document.title = 'CLEAN:%1';").arg(QFileInfo(filePath).fileName())
//       );

//       codeModifiedFlag = false;
//       updateTitle();
//     } else {
//       qDebug() << "Could not open file for writing:" << file.errorString();
//     }
//   }
// }

// void MainWindow::closeEvent(QCloseEvent *event) {

//   if (codeModifiedFlag) {

//     auto result = QMessageBox::question(this, "Unsaved changes",
//                 "File has unsaved changes. Save before closing?",
//                 QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

//     if (result == QMessageBox::Save) {
//       saveCurrentCode();
//       event->accept();
//     } else if (result == QMessageBox::Discard) {
//       event->accept();
//     } else {
//       event->ignore();
//     }

//   } else {
//     event->accept();
//   }
// }

// MainWindow::~MainWindow()
// {
//     // delete ui;
// }


#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    auto *central = new QWidget(this);
    setCentralWidget(central);

    pathEdit_ = new QLineEdit("/tmp/test.txt");
    loadBtn_  = new QPushButton("Загрузить");
    saveBtn_  = new QPushButton("Сохранить");
    editor_   = new QTextEdit;

    auto *top = new QHBoxLayout;
    top->addWidget(pathEdit_);
    top->addWidget(loadBtn_);
    top->addWidget(saveBtn_);

    auto *layout = new QVBoxLayout(central);
    layout->addLayout(top);
    layout->addWidget(editor_);

    resize(600, 400);

    connect(loadBtn_, &QPushButton::clicked, this, &MainWindow::onLoad);
    connect(saveBtn_, &QPushButton::clicked, this, &MainWindow::onSave);

    sock_ = new QTcpSocket(this);
    connect(sock_, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
    sock_->connectToHost("127.0.0.1", 5000);
}

void MainWindow::onLoad() {
    Message msg;
    msg.flag = "load";
    msg.payload["path"] = pathEdit_->text();
    send(msg);
}

void MainWindow::onSave() {
    Message msg;
    msg.flag = "save";
    msg.payload["path"]    = pathEdit_->text();
    msg.payload["content"] = editor_->toPlainText();
    send(msg);
}

void MainWindow::send(const Message &msg) {
    if (sock_->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, "Ошибка", "Нет соединения с сервером");
        return;
    }
    sock_->write(msg.serialize());
}

void MainWindow::onReadyRead() {
    buffer_.append(sock_->readAll());

    Message msg;
    while (Message::tryDeserialize(buffer_, msg)) {
        if (msg.flag == "output") {
            if (msg.payload["ok"].toBool()) {
                // для load — показываем содержимое; для save — просто ok
                if (msg.payload.contains("content"))
                    editor_->setPlainText(msg.payload["content"].toString());
                else
                    statusBar()->showMessage("Сохранено", 2000);
            }
        } else if (msg.flag == "error") {
            QMessageBox::critical(this, "Ошибка сервера",
                                  msg.payload["message"].toString());
        }
    }
}
