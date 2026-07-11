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
  void loadLocalFile(const QString &filePath);

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
  void updateTitle();
  void updateFileTree();
  void loadDirectory(const QString &path, QTreeWidgetItem *parent = nullptr);
  void runJS(const QString &js,
             std::function<void(const QVariant &)> callback = nullptr);
  void send(const Message &msg);
  void handleNetworkCommand(const QString &cmd);

  QWebEngineView *EditorSpace;
  QSplitter *mainSplitter;
  QSplitter *editorSplitter;
  QString currentFilePath;
  QString currentPath;
  bool codeModifiedFlag = false;
  bool monacoReady = false;
  QTreeWidget *fileTree;

  QTextEdit *console;
  QLineEdit *commandEdit;
  QWidget *consoleWidget;
  QLineEdit *hostEdit;
  QSpinBox *portSpin;
  QPushButton *connBtn;
  QMenu *connMenu;
  QProcess *shellProcess;

  QTcpSocket *sock;
  QByteArray buffer;

protected:
  void closeEvent(QCloseEvent *event) override;
};

#endif // MAINWINDOW_H
