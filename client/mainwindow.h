// #ifndef MAINWINDOW_H
// #define MAINWINDOW_H

// #include <QMainWindow>
// class QWebEngineView;
// class QTreeWidget;
// class QTreeWidgetItem;

// class MainWindow : public QMainWindow {
//   Q_OBJECT

// public:
//   MainWindow(QWidget *parent = nullptr);
//   ~MainWindow();

//   QString getCode();
//   void loadFile(const QString &filepath);

// private slots:
//   void onOpenFile();
//   void saveCodeAs();
//   void saveCurrentCode();
//   void saveCode(QString filePath);

// private:
//   // Ui::MainWindow *ui;
//   void updateTitle();
//   void updateFileTree();
//   void loadDirectory(const QString &path, QTreeWidgetItem *parent = nullptr);

//   QWebEngineView *EditorSpace;
//   QString currentFilePath;
//   QString currentPath;
//   bool codeModifiedFlag = false;
//   bool monacoReady = false;
//   QTreeWidget *fileTree;




// protected:
//     void closeEvent(QCloseEvent *event) override;

// };

// #endif // MAINWINDOW_H


#pragma once
#include <QMainWindow>
#include <QTcpSocket>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include "message.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onLoad();
    void onSave();
    void onReadyRead();

private:
    QTcpSocket *sock_;
    QByteArray  buffer_;
    QLineEdit  *pathEdit_;
    QTextEdit  *editor_;
    QPushButton *loadBtn_, *saveBtn_;

    void send(const Message &msg);
};
