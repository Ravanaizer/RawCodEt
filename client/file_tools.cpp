#include "mainwindow.h"


// Select file for open and load
void MainWindow::onOpenFile() {
  QString filePath = QFileDialog::getOpenFileName(
      this, "Open file", m_currentPath,
      "Все файлы (*.*);;C++ файлы (*.cpp *.h *.hpp);;Python "
      "(*.py);;JavaScript(*.js);; HTML(*.html)");

  if (!filePath.isEmpty()) {
    loadLocalFile(filePath);
  }

  m_currentPath = QFileInfo(filePath).absolutePath();
  updateFileTree();
}

// Load file from local storage
void MainWindow::loadLocalFile(const QString &filePath) {
  QFile file(filePath);

  QString content = "";

  if (file.open(QIODevice::ReadOnly)) {
    QByteArray rawData = file.readAll();
    file.close();

    content = QString::fromUtf8(rawData);
  }

  loadFile(filePath, content);
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

  m_currentFilePath = filePath;
  m_codeModifiedFlag = false;
  updateWindowTitle();
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
      this, "Open file", m_currentPath,
      "Все файлы (*.*);;C++ файлы (*.cpp *.h *.hpp);;Python (*.py);;JavaScript "
      "(*.js);; HTML(*.html)");

  saveCode(filePath);
}

// Save code to current file
void MainWindow::saveCurrentCode() { MainWindow::saveCode(m_currentFilePath); }

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

      m_codeModifiedFlag = false;
      updateWindowTitle();
    } else {
      qDebug() << "Could not open file for writing:" << file.errorString();
    }
  }
}
