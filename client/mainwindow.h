#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
class QWebEngineView;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  QString getCode();
  void loadFile(const QString &filepath);

private slots:
  void onOpenFile();
  void saveCodeAs();
  void saveCurrentCode();
  void saveCode(QString filePath);

private:
//     Ui::MainWindow *ui;
  void updateTitle();

  QWebEngineView *EditorSpace;
  QString currentFilePath;
  bool codeModifiedFlag = false;
};

#endif // MAINWINDOW_H
