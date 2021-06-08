#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QQuickWindow>
#include <QQuickWidget>
#include <QListWidget>
#include <canvas/vcanvas.h>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget *parent = nullptr);
        ~MainWindow();
        bool event(QEvent *e) override;
    private slots:
        void quickWidgetStatusChanged(QQuickWidget::Status);
        void sceneGraphError(QQuickWindow::SceneGraphError error, const QString &message);
        void updateLayers();
        void updateMode();
        void updateSidePanel();
        void openFile();
        void openImageFile();
        void layerOrderChanged(const QModelIndex & sourceParent, int sourceStart, int sourceEnd, const QModelIndex & destinationParent, int destinationRow);
        void on_addLayer_clicked();

    private:
        Ui::MainWindow *ui;
        VCanvas *canvas_;
        QListWidget *layer_list_;
};

#endif // MAINWINDOW_H
