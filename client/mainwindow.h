#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "message.h"
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
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTcpSocket>
#include <QTextEdit>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include <QWidgetAction>

// Forward declarations to reduce compilation time
class QWebEngineView;
class QTreeWidget;
class QTreeWidgetItem;

/**
 * @brief The MainWindow class represents the main application window.
 *
 * It integrates a Monaco code editor (via QWebEngineView), a local file tree,
 * a console for local shell and remote network commands, and a TCP client
 * for interacting with a remote compiler server.
 */
class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  /**
   * @brief Retrieves the current code text from the Monaco editor.
   * @note This is a blocking call that uses QEventLoop to wait for JS
   * execution.
   * @return The source code as a QString.
   */
  QString getCode();

  /**ф
   * @brief Loads code and metadata into the editor.
   * @param filePath The path of the file (used to determine language and
   * title).
   * @param content The actual source code text.
   */
  void loadFile(const QString &filePath, const QString &content);

private slots:
  // --- File Operations ---
  void onOpenFile();
  void saveCodeAs();
  void saveCurrentCode();
  void saveCode(QString filePath);

  // --- Remote Network Operations ---
  void onRemoteLoad(QString filePath);
  void onRemoteSave(QString filePath);
  void remoteGetDirs(QString path);

  // --- Console & Network Events ---
  void onReadyRead();
  void onCommandEntered();

private:
  // --- UI Setup & Helpers ---
  void loadLocalFile(const QString &filePath);
  void setupUI();
  void setupConnects();
  void updateWindowTitle();
  void updateFileTree();
  void loadDirectoryForFileTree(const QString &path,
                                QTreeWidgetItem *parent = nullptr);

  /**
   * @brief Executes JavaScript in the QWebEngineView.
   * @param js The JS code to execute.
   * @param callback Optional callback to handle the JS return value.
   */
  void runJS(const QString &js,
             std::function<void(const QVariant &)> callback = nullptr);

  // --- Network Helpers ---
  void send(const Message &msg);
  void handleNetworkCommand(const QString &cmd);

  // --- Member Variables ---
  // State
  QString          m_currentPath;
  bool             m_codeModifiedFlag = false;
  bool             m_monacoReady = false;
  QString          m_currentFilePath;

  // UI Components
  QWebEngineView   *m_EditorSpace;
  QSplitter        *m_mainSplitter;
  QSplitter        *m_editorSplitter;
  QTreeWidget      *m_fileTree;
  QTextEdit        *m_console;
  QLineEdit        *m_commandEdit;
  QWidget          *m_consoleWidget;

  // Local Shell
  QProcess         *m_shellProcess;

  // Network / Remote Compiler
  QLineEdit        *m_hostEdit;
  QSpinBox         *m_portSpin;
  QPushButton      *m_connBtn;
  QMenu            *m_connMenu;
  QAction          *m_connectBtnAction;
  QTcpSocket       *m_sock;
  QByteArray       m_buffer; // Buffer for accumulating partial TCP messages

protected:
  void closeEvent(QCloseEvent *event) override;
};

#endif // MAINWINDOW_H
