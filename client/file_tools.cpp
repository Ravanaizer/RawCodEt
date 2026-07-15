#include "mainwindow.h"

// Opens a file dialog and loads the selected file into the editor
void MainWindow::onOpenFile() {
  QString filePath = QFileDialog::getOpenFileName(
      this, "Open file", m_currentPath,
      "All files (*.*);;C++ files (*.cpp *.h *.hpp);;Python "
      "(*.py);;JavaScript(*.js);; HTML(*.html)");

  if (!filePath.isEmpty()) {
    loadLocalFile(filePath);
  }
  // Update current working directory for the file tree
  m_currentPath = QFileInfo(filePath).absolutePath();
  updateFileTree();
}

// Reads file content from the local disk and passes it to the loader
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

// Core loading logic: pushes code to Monaco Editor via JavaScript
void MainWindow::loadFile(const QString &filePath, const QString &content) {
  // Encode to Base64 to safely pass special characters (quotes, newlines) to JS
  QString base64Content = content.toUtf8().toBase64();
  runJS(QString("editor.setValue(atob('%1'));").arg(base64Content));

  // Determine language mode for syntax highlighting
  QString lang = "cpp";
  if (filePath.endsWith(".py"))
    lang = "python";
  else if (filePath.endsWith(".js"))
    lang = "javascript";
  else if (filePath.endsWith(".html"))
    lang = "html";

  runJS(QString("monaco.editor.setModelLanguage(editor.getModel(),'%1');")
            .arg(lang));

  // Update internal state and JS global variables
  runJS(QString("window.currentFileName ='%1';")
            .arg(QFileInfo(filePath).fileName()));
  runJS("window.setupChangeHandler();"); // Re-attach dirty state listeners

  m_currentFilePath = filePath;
  m_codeModifiedFlag = false;
  updateWindowTitle();
}

// Retrieves current code from Monaco Editor synchronously
QString MainWindow::getCode() {
  QString code;
  QEventLoop loop; // Used to block until the async JS callback returns

  // Ask JS to encode the editor content to Base64 and return it
  runJS("btoa(editor.getValue());", [&code, &loop](const QVariant &result) {
    // Decode Base64 back to UTF-8 string
    code = QByteArray::fromBase64(result.toString().toUtf8());
    loop.quit(); // Exit the event loop
  });

  loop.exec(); // Block execution here until JS finishes
  return code;
}

// Opens a dialog to save the current code to a new file
void MainWindow::saveCodeAs() {
  QString filePath =
      QFileDialog::getSaveFileName( // Note: changed to getSaveFileName for
                                    // "Save As" logic
          this, "Save file", m_currentPath,
          "All files (*.*);;C++ files (*.cpp *.h *.hpp);;Python "
          "(*.py);;JavaScript (*.js);; HTML(*.html)");
  saveCode(filePath);
}

// Saves the current code to the currently open file path
void MainWindow::saveCurrentCode() { MainWindow::saveCode(m_currentFilePath); }

// Core saving logic: writes code to disk and resets the "dirty" state
void MainWindow::saveCode(QString filePath) {
  if (!filePath.isEmpty()) {
    QString codeText = getCode();
    QFile file(filePath);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      file.write(codeText.toUtf8());
      file.close();

      // Tell Monaco that the content is now "clean" (saved)
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
