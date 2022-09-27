#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QQuickWindow>
#include <QQuickWidget>
#include <QComboBox>
#include <QPushButton>
#include <QListWidget>
#include <QMenu>
#include <QToolButton>
#include <widgets/components/layer-list-item.h>
#include <widgets/panels/transform-panel.h>
#include <widgets/panels/doc-panel.h>
#include <widgets/panels/layer-panel.h>
#include <widgets/panels/font-panel.h>
#include <widgets/panels/image-panel.h>
#include <widgets/panels/jogging-panel.h>
#include <widgets/panels/laser-panel.h>
#include <windows/machine-manager.h>
#include <windows/preferences-window.h>
#include <windows/gcode-panel.h>
#include <windows/welcome-dialog.h>
#include <canvas/canvas.h>
#include <widgets/base-container.h>

#include <windows/job-dashboard-dialog.h>
#include <windows/about-window.h>
#include <windows/privacy_window.h>
#include <executor/executor.h>
#include <windows/rotary_setup.h>

#ifdef ENABLE_SENTRY
#include <sentry.h>
#endif

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

  MachineSettings::MachineSet currentMachine();

signals:

  void presetSettingsChanged();

  void machineSettingsChanged();

  void toolbarTransformChanged(double x, double y, double r, double w, double h);

public slots:
  void onStartNewJob();
  void onStopJob();
  void onPauseJob();
  void onResumeJob();

  // Launch simple job (e.g. from jogging panel)
  void laser(qreal power);
  void laserPulse(qreal power);
  void home();
  void moveRelatively(qreal x, qreal y, qreal feedrate);
  void moveToEdge(int edge_id, qreal feedrate);
  void moveToCorner(int corner_id, qreal feedrate);


private slots:

  void canvasLoaded(QQuickWidget::Status status);

  void sceneGraphError(QQuickWindow::SceneGraphError error, const QString &message);

  void updateMode();

  void updateSelections();

  void updateScale();

  void updateToolbarTransform();

  void onScaleMinusClicked();

  void onScalePlusClicked();

  void openFile();

  void openExampleOfSwiftray();

  void openMaterialCuttingTest();

  void openMaterialEngravingTest();

  void openImageFile();

  void replaceImage();

  void imageSelected(const QImage image);

  void importImage(QString file_name);

  void setCanvasContextMenu();

  void setConnectionToolBar();

  void setToolbarFont();

  void setToolbarTransform();

  //void setToolbarImage();

  void setModeBlock();
  
  void setScaleBlock();

  void showCanvasPopMenu();

  void showWelcomeDialog();

  void showJoggingPanel();

  void genPreviewWindow();

  void syncJobState(Executor::State state);

  void jobDashboardFinish(int result);

  void updateTitle(bool file_modified);

  void updateRotary();

private:

  void loadWidgets() override;

  void loadSettings() override;

  void registerEvents() override;

  void resizeEvent(QResizeEvent *event) override;

  Ui::MainWindow *ui;
  Canvas *canvas_;
  double x_, y_, r_, w_, h_;
  bool job_dashboard_exist_;
  bool is_high_speed_mode_ = false;
  bool is_upload_enable_ = false;
  bool is_rotary_mode_ = false;
  bool is_mirror_mode_ = false;
  QString rotary_axis_ = "Y";
  double current_x_ = 0, current_y_ = 0;//unit??
#ifdef ENABLE_SENTRY
  sentry_options_t *options_;
#endif
  QString current_filename_;

  // Context menu of canvas
  QMenu *popMenu_;
  QAction *cutAction_;
  QAction *copyAction_;
  QAction *pasteAction_;
  QAction *pasteInPlaceAction_;
  QAction *duplicateAction_;
  QAction *deleteAction_;
  QAction *groupAction_;
  QAction *ungroupAction_;

  QPushButton *scale_block_;
  QToolButton *minusBtn_;
  QToolButton *plusBtn_;
  QMenu *popScaleMenu_;
  QComboBox* baudComboBox_;
  QComboBox* portComboBox_;
  QPushButton *mode_block_;
  QMenu *popModeMenu_;

  TransformPanel *transform_panel_;
  GCodePanel *gcode_panel_;
  JobDashboardDialog *job_dashboard_;
  DocPanel *doc_panel_;
  FontPanel *font_panel_;
  ImagePanel *image_panel_;
  LayerPanel *layer_panel_;
  LaserPanel *laser_panel_;
  MachineManager *machine_manager_;
  WelcomeDialog *welcome_dialog_;
  JoggingPanel *jogging_panel_;
  PreferencesWindow *preferences_window_;
  AboutWindow *about_window_;
  PrivacyWindow *privacy_window_;
  RotarySetup *rotary_setup_;

#ifndef Q_OS_IOS
#endif

  void newFile();
  void saveFile();
  bool saveAsFile();
  void exportGCodeFile();
  void importGCodeFile();
  bool generateGcode();
  bool handleUnsavedChange();
  void actionStart();
  void actionFrame();
  QTransform calculateTranslate();
};

#endif // MAINWINDOW_H
