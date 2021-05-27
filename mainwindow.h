#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QQuickWindow>
#include <QQuickWidget>
#include "vcanvas.h"
#include "vdoc.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = nullptr);
        ~MainWindow();

    private slots:
        void quickWidgetStatusChanged(QQuickWidget::Status);
        void sceneGraphError(QQuickWindow::SceneGraphError error, const QString &message);
        void openFile();


    private:
        Ui::MainWindow *ui;
        VDoc *doc_;
        VCanvas *canvas_;
};

#endif // MAINWINDOW_H
