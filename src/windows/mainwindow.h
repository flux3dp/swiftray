#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QQuickWindow>
#include <QQuickWidget>
#include <QListWidget>
#include <QToolButton>
#include <widgets/components/layer-list-item.h>
#include <widgets/panels/transform-panel.h>
#include <widgets/panels/doc-panel.h>
#include <widgets/panels/layer-panel.h>
#include <widgets/panels/font-panel.h>
#include <windows/machine-manager.h>
#include <windows/preferences-window.h>
#include <windows/gcode-player.h>
#include <windows/welcome-dialog.h>
#include <canvas/canvas.h>
#include <widgets/base-container.h>

namespace Ui {
  class MainWindow;
}

class MainWindow : public QMainWindow, BaseContainer {
Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);

  ~MainWindow();

  bool event(QEvent *e) override;

  void loadCanvas();

  void loadStyles() override;

  void closeEvent(QCloseEvent *event) override;

  Canvas *canvas() const;

signals:

  void presetSettingsChanged();

  void machineSettingsChanged();

private slots:

  void canvasLoaded(QQuickWidget::Status status);

  void sceneGraphError(QQuickWindow::SceneGraphError error, const QString &message);

  void updateMode();

  void updateSelections();

  void openFile();

  void openImageFile();

  void imageSelected(const QImage image);

  void showWelcomeDialog();

private:

  void loadWidgets() override;

  void loadSettings() override;

  void registerEvents() override;

  Ui::MainWindow *ui;
  Canvas *canvas_;
  TransformPanel *transform_panel_;
  GCodePlayer *gcode_player_;
  DocPanel *doc_panel_;
  FontPanel *font_panel_;
  LayerPanel *layer_panel_;
  MachineManager *machine_manager_;
  WelcomeDialog *welcome_dialog_;
  PreferencesWindow *preferences_window_;

  void saveFile();
};

#endif // MAINWINDOW_H
