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
  void saveCode();

private:
//     Ui::MainWindow *ui;
    QWebEngineView *EditorSpace;
};

#endif // MAINWINDOW_H
