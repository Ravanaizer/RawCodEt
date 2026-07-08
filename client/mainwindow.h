#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QMenu>
#include <QSpinBox>

#include "message.h"

class QWebEngineView;
class QTreeWidget;
class QTreeWidgetItem;


class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  QString getCode();
  void loadFile(const QString &filepath);

private slots:
  void onOpenFile();
  void saveCodeAs();
  void saveCurrentCode();
  void saveCode(QString filePath);
  void onRemoteLoad(QString filePath);
  void onRemoteSave(QString filePath);
  void remoteGetDirs(QString path);
  // void onReadyRead();
  // void onCommandEntered();

private:
  // Ui::MainWindow *ui;
  void updateTitle();
  void updateFileTree();
  void loadDirectory(const QString &path, QTreeWidgetItem *parent = nullptr);
  void runJS(const QString &js, std::function<void(const QVariant&)> callback = nullptr);
  void send(const Message &msg);

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

  QTcpSocket *sock;
  QByteArray  buffer;

protected:
    void closeEvent(QCloseEvent *event) override;

};

#endif // MAINWINDOW_H


// #pragma once
// #include <QMainWindow>
// #include <QTcpSocket>
// #include <QLineEdit>
// #include <QTextEdit>
// #include <QPushButton>
//

// class MainWindow : public QMainWindow {
//     Q_OBJECT
// public:
//     explicit MainWindow(QWidget *parent = nullptr);

// private slots:
//     void onLoad();
//     void onSave();
//     void onReadyRead();

// private:
//     QLineEdit  *pathEdit_;
//     QTextEdit  *editor_;
//     QPushButton *loadBtn_, *saveBtn_;

// };
