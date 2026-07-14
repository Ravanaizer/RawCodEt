#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "message.h"
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QProcess>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QTcpSocket>
#include <QTextEdit>
#include <QTreeWidget>
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

class QWebEngineView;
class QTreeWidget;
class QTreeWidgetItem;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  QString getCode();
  void loadFile(const QString &filePath, const QString &content);


private slots:
  void onOpenFile();
  void saveCodeAs();
  void saveCurrentCode();
  void saveCode(QString filePath);
  void onRemoteLoad(QString filePath);
  void onRemoteSave(QString filePath);
  void remoteGetDirs(QString path);
  void onReadyRead();
  void onCommandEntered();

private:
  // Ui::MainWindow *ui;
  void loadLocalFile(const QString &filePath);
  void setupUI();
  void setupConnects();
  void updateWindowTitle();
  void updateFileTree();
  void loadDirectoryForFileTree(const QString &path, QTreeWidgetItem *parent = nullptr);
  void runJS(const QString &js,
             std::function<void(const QVariant &)> callback = nullptr);
  void send(const Message &msg);
  void handleNetworkCommand(const QString &cmd);

  QString          m_currentPath;
  bool             m_codeModifiedFlag = false;
  bool             m_monacoReady = false;

  QWebEngineView   *m_EditorSpace;
  QSplitter        *m_mainSplitter;
  QSplitter        *m_editorSplitter;
  QString          m_currentFilePath;

  QTreeWidget      *m_fileTree;

  QTextEdit        *m_console;
  QLineEdit        *m_commandEdit;
  QWidget          *m_consoleWidget;
  QProcess         *m_shellProcess;

  QLineEdit        *m_hostEdit;
  QSpinBox         *m_portSpin;
  QPushButton      *m_connBtn;
  QMenu            *m_connMenu;
  QAction          *m_connectBtnAction;

  QTcpSocket       *m_sock;
  QByteArray       m_buffer;

protected:
  void closeEvent(QCloseEvent *event) override;
};

#endif // MAINWINDOW_H
