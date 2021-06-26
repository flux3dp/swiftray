#include <QMainWindow>
#include <QQuickWindow>
#include <QQuickWidget>
#include <QListWidget>
#include <QToolButton>
#include <widgets/components/layer-list-item.h>
#include <widgets/panels/transform-panel.h>
#include <widgets/panels/layer-params-panel.h>
#include <windows/gcode-player.h>
#include <widgets/panels/font-panel.h>
#include <canvas/canvas.h>

#ifndef MAINWINDOW_H
#define MAINWINDOW_H


namespace Ui {
  class MainWindow;
}

class MainWindow : public QMainWindow {
Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);

  ~MainWindow();

  bool event(QEvent *e) override;

  void loadCanvas();

  void loadQSS();

  void loadWidgets();

private slots:

  void canvasLoaded(QQuickWidget::Status status);

  void sceneGraphError(QQuickWindow::SceneGraphError error, const QString &message);

  void updateLayers();

  void updateMode();

  void updateSelections();

  void openFile();

  void openImageFile();

  void registerEvents();

  void layerOrderChanged(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                         const QModelIndex &destinationParent, int destinationRow);

private:
  Ui::MainWindow *ui;
  Canvas *canvas_;
  unique_ptr<LayerParamsPanel> layer_params_panel_;
  unique_ptr<TransformPanel> transform_panel_;
  unique_ptr<GCodePlayer> gcode_player_;
  unique_ptr<FontPanel> font_panel_;
  unique_ptr<QToolButton> add_layer_btn_;
};

#endif // MAINWINDOW_H
