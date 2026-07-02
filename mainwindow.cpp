#include "mainwindow.h"
// #include "ui_mainwindow.h"
#include <QWebEngineView>
#include <QWebEngineSettings>
#include <QCoreApplication>
#include <QDir>
#include <QUrl>
#include <QFileInfo>
#include <QDebug>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonValue>
#include <QMenuBar>
#include <QAction>




MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    // , ui(new Ui::MainWindow)
{
    // ui->setupUi(this);

    EditorSpace = new QWebEngineView(this);

    QWebEngineSettings *settings = EditorSpace->settings();
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);

    QString editorHtmlPath = QCoreApplication::applicationDirPath() + "/editor.html";

    EditorSpace->setUrl(QUrl::fromLocalFile(editorHtmlPath));
    setCentralWidget(EditorSpace);

    qDebug() << "Find:" << editorHtmlPath;
    qDebug() << "File expected:" << QFile::exists(editorHtmlPath);

    QMenu *fileMenu = menuBar()->addMenu("File");
    QAction *openFileAction = fileMenu->addAction("Open");
    openFileAction -> setShortcut(QKeySequence(Qt::CTRL | Qt::Key_O));
    connect(openFileAction, &QAction::triggered, this, &MainWindow::onOpenFile);
    QAction *saveFileActionAs = fileMenu->addAction("Save");
    saveFileActionAs -> setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S | Qt::SHIFT));
    connect(saveFileActionAs, &QAction::triggered, this, &MainWindow::saveCode);

    resize(1200, 800);
    setWindowTitle("RawCodEt - Monaco Editor");
}

void MainWindow::onOpenFile() {
  QString filePath = QFileDialog::getOpenFileName(
    this,
    "Open file",
    QDir::currentPath(),
    "Все файлы (*.*);;C++ файлы (*.cpp *.h *.hpp);;Python (*.py);;JavaScript (*.js);;HTML (*.html)"
  );

  if (!filePath.isEmpty()) {
    loadFile(filePath);
  }
}

void MainWindow::loadFile(const QString &filePath) {
  QFile file(filePath);

  if (file.open(QIODevice::ReadOnly)) {
    // Get byte array from file and close file
    QByteArray rawData = file.readAll();
    file.close();

    // Byte array to UTF-8 string
    QString content = QString::fromUtf8(rawData);

    // UTF-8 string to JSON string
    QJsonValue jsonVal(content);
    QString jsonStr = QString::fromUtf8(jsonVal.toJson(QJsonDocument::Compact));

    // Set JSON string to editor
    EditorSpace->page()->runJavaScript(QString("editor.setValue(%1);").arg(jsonStr));

    // Select syntax highlighting
    QString lang = "cpp";
    if (filePath.endsWith(".py")) lang = "python";
    else if (filePath.endsWith(".js")) lang = "javascript";
    else if (filePath.endsWith(".html")) lang = "html";

    EditorSpace->page()->runJavaScript(QString("monaco.editor.setModelLanguage(editor.getModel(), '%1');").arg(lang));

    setWindowTitle("RawCodEt - " + QFileInfo(filePath).fileName());
    qDebug() << "File is open successfully:" << filePath;
  } else {
    qDebug() << "Error:" << filePath;
  }
}

QString MainWindow::getCode()
{
    QString code;
    QEventLoop loop;
    EditorSpace->page()->runJavaScript("editor.getValue();", [&code, &loop](const QVariant &result) {
        code = result.toString();
        loop.quit();
    });
    loop.exec();
    return code;
}

void MainWindow::saveCode() {
  QString filePath = QFileDialog::getOpenFileName(
    this,
    "Open file",
    QDir::currentPath(),
    "Все файлы (*.*);;C++ файлы (*.cpp *.h *.hpp);;Python (*.py);;JavaScript (*.js);;HTML (*.html)"
  );

  if (!filePath.isEmpty()) {
    QString codeText = getCode();

    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      file.write(codeText.toUtf8());
      file.close();
    } else {
      qDebug() << "Could not open file for writing:" << file.errorString();
    }
  }
}

MainWindow::~MainWindow()
{
    // delete ui;
}
