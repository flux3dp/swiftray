#include <QDebug>
#include <QFileDialog>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickWidget>
#include <QDir>
#include <QFont>
#include <QFontComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QToolButton>
#include <QCheckBox>
#include <QMessageBox>
#include <QTemporaryDir>
#include <constants.h>
#include <QMessageBox>
#include <QProgressDialog>
#include <widgets/components/qdoublespinbox2.h>
#include <shape/bitmap-shape.h>
#include <widgets/components/canvas-text-edit.h>
#include <windows/mainwindow.h>
#include <windows/osxwindow.h>
#include <toolpath_exporter/toolpath-exporter.h>
#include <toolpath_exporter/generators/gcode-generator.h>
#include <toolpath_exporter/generators/preview-generator.h>
#include <toolpath_exporter/generators/dirty-area-outline-generator.h>
#include <document-serializer.h>
#include <settings/file-path-settings.h>
#include <windows/preview-window.h>
#include <windows/job-dashboard-dialog.h>
#include <globals.h>
#include "widgets/components/canvas-widget.h"
#include <executor/machine_job/gcode_job.h>
#include <common/timestamp.h>
#include <QSerialPort>
#include <main_application.h>
#include "parser/pdf2svg.h"
#include <windows/preset-manager.h>
#include <settings/machine-settings.h>
#include <settings/preset-settings.h>

#include <QResource>
#include <utils/software_update.h>
#include "config.h"

#include "ui_mainwindow.h"

#define xstr(s) str(s)
#define str(s)  #s

MainWindow::MainWindow(QWidget *parent) :
     QMainWindow(parent),
     ui(new Ui::MainWindow),
     canvas_(nullptr),
     job_dashboard_exist_(false),
     BaseContainer() {
  ui->setupUi(this);
  loadCanvas();
  initializeContainer();
  updateMode();
  setCanvasContextMenu();
  setToolbarFont();
  setToolbarTransform();
  //setToolbarImage();
  Q_EMIT canvas_->selectionsChanged(QList<ShapePtr>());
  setScaleBlock();
  setModeBlock();
  setConnectionToolBar();
}

void MainWindow::show() {
  QMainWindow::show();
  if (layer_panel_) {
    layer_panel_->resizeEvent(nullptr);
  }
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    // showEvent() is called right *before* the window is shown, but WinSparkle
    // requires that the main UI of the application is already shown when
    // calling win_sparkle_init() (otherwise it could show its updates UI
    // behind the app instead of at top). By using a helper signal, we delay
    // calling initWinSparkle() / initSparkle() until after the window was shown.
    //
    // Alternatively, one could achieve the same effect in arguably a simpler
    // way, by initializing WinSparkle in main(), right after showing the main
    // window. See https://github.com/vslavik/winsparkle/issues/41#issuecomment-70367197
    // for a discussion of this.
    Q_EMIT windowWasShown();
}

void MainWindow::loadSettings() {
  #ifdef Q_OS_MACOS
    QSettings settings("flux", "swiftray");
    restoreGeometry(settings.value("window/geometry").toByteArray());
    if (!restoreState(settings.value("window/windowState").toByteArray())) {
      QSettings classic_settings(":/classicUI.ini", QSettings::IniFormat);
      restoreState(classic_settings.value("window/windowState").toByteArray());
    };
  #else
    QSettings settings(":/classicUI.ini", QSettings::IniFormat);
    restoreState(settings.value("window/windowState").toByteArray());
  #endif
  //panel
  ui->actionObjectPanel->setChecked(!ui->objectParamDock->isHidden());
  ui->actionFontPanel->setChecked(!ui->fontDock->isHidden());
  ui->actionImagePanel->setChecked(!ui->imageDock->isHidden());
  ui->actionDocumentPanel->setChecked(!ui->documentDock->isHidden());
  ui->actionLayerPanel->setChecked(!layer_panel_->isHidden());
  ui->actionGCodeViewerPanel->setChecked(!gcode_panel_->isHidden());
  ui->actionJoggingPanel->setChecked(!jogging_panel_->isHidden());
  ui->actionLaser->setChecked(!laser_panel_->isHidden());
  //toolbar
  ui->actionAlign->setChecked(!ui->toolBarAlign->isHidden());
  ui->actionBoolean->setChecked(!ui->toolBarBool->isHidden());
  ui->actionConnection->setChecked(!ui->toolBarConnection->isHidden());
  ui->actionFlip->setChecked(!ui->toolBarFlip->isHidden());
  ui->actionFont->setChecked(!ui->toolBarFont->isHidden());
  ui->actionFunctional->setChecked(!ui->toolBar->isHidden());
  ui->actionGroup_2->setChecked(!ui->toolBarGroup->isHidden());
  ui->actionImage->setChecked(!ui->toolBarImage->isHidden());
  ui->actionProject->setChecked(!ui->toolBarFile->isHidden());
  ui->actionTask->setChecked(!ui->toolBarTask->isHidden());
  ui->actionTransform->setChecked(!ui->toolBarTransform->isHidden());
  ui->actionVector->setChecked(!ui->toolBarVector->isHidden());
  
  preferences_window_->setSpeedMode(mainApp->isHighSpeedMode());
  preferences_window_->setCanvasQuality(mainApp->getCanvasQuality());
  preferences_window_->setPathSort(mainApp->getPathSort());
  canvas_->setCanvasQuality(mainApp->getCanvasQuality());
  setWindowModified(false);
  setWindowFilePath(FilePathSettings::getDefaultFilePath());
  setWindowTitle(tr("Untitled") + " - Swiftray");
  current_filename_ = tr("Untitled");
  MachineSettings::MachineParam machine_param = mainApp->getMachineParam();
  canvas_->setCurrentPosition(jogging_panel_->getShowCurrent());
  canvas_->setUserOrigin(jogging_panel_->getShowUserOrigin());
  canvas_->transformControl().setScaleLock(mainApp->isShapeScaleLocked());
  canvas_->setShapeReference(mainApp->getShapeReference());
  laser_panel_->setJobOrigin(mainApp->getJobOrigin());
  laser_panel_->setStartFrom(mainApp->getStartFrom());
  laser_panel_->setStartHome(mainApp->getStartWithHome());
  layer_panel_->setPresetIndex(mainApp->getPresetIndex(), mainApp->getParamIndex());
  //should update by layer!!!
  PresetSettings::Param initial_param;
  layer_panel_->setLayerParam(initial_param.power, initial_param.speed, initial_param.repeat);
  layer_panel_->setLayerBacklash(0);
  doc_panel_->setPresetIndex(mainApp->getPresetIndex());
  jogging_panel_->setFramingPower(mainApp->getFramingPower());
  jogging_panel_->setPulsePower(mainApp->getPulsePower());
  doc_panel_->setMachineIndex(mainApp->getMachineIndex());
  doc_panel_->setTravelSpeed(machine_param.travel_speed);
  RotarySettings::RotaryParam rotary_param = mainApp->getRotaryParam();
  doc_panel_->setRotarySpeed(rotary_param.travel_speed);
  updateCanvasSize(mainApp->getWorkingRange());
  //should check when machine(which is connected?) change
  jogging_panel_->setControlEnable(false);
  laser_panel_->setControlEnable(false);
#ifdef ENABLE_SENTRY
  // Launch Crashpad with Sentry
  options_ = sentry_options_new();
  QString database_path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/sentry-native";
  sentry_options_set_database_path(options_, database_path.toStdString().c_str());
  std::string dsn_string(xstr(ENABLE_SENTRY));
  dsn_string = "https://" + dsn_string;
  sentry_options_set_dsn(options_, dsn_string.c_str());
  #ifdef Q_OS_MACOS
  //qInfo() << "Crashpad path" << QCoreApplication::applicationDirPath().append("/../Resources/crashpad_handler");
  sentry_options_set_handler_path(options_,
      QCoreApplication::applicationDirPath().toStdString().append("/crashpad_handler").c_str());
  #else
  //qInfo() << "Crashpad path" << QCoreApplication::applicationDirPath().append("/crashpad_handler.exe");
  sentry_options_set_handler_path(options_,
      QCoreApplication::applicationDirPath().toStdString().append("/crashpad_handler.exe").c_str());
  #endif
  // sentry_options_set_debug(options_, 1); // More details for debug
  sentry_options_set_release(options_,
      std::string("Swiftray@")
      .append(std::to_string(VERSION_MAJOR))
      .append(".")
      .append(std::to_string(VERSION_MINOR))
      .append(".")
      .append(std::to_string(VERSION_BUILD))
      .append(VERSION_SUFFIX)
      .c_str()
  );
  sentry_options_set_require_user_consent(options_, true);
  sentry_init(options_);
  if(mainApp->isUploadEnable()) {
    sentry_user_consent_give();
  }
  else {
    sentry_user_consent_revoke();
  }
#endif
  if(mainApp->isFirstTime()) {
    welcome_dialog_->setWindowModality(Qt::ApplicationModal);
    welcome_dialog_->show();
    welcome_dialog_->activateWindow();
    welcome_dialog_->raise();
  }
  preferences_window_->setUpload(mainApp->isUploadEnable());
}

void MainWindow::loadCanvas() {
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
  // QPaintedItem in Qt6 does not support Metal rendering yet, so it will be slow using metal RHI
  qInfo() << "Falling back to OpenGLRhi in QT6";
  ((QQuickWindow *)ui->quickWidget)->setGraphicsApi(QSGRendererInterface::OpenGLRhi);
  connect(ui->quickWidget, &QQuickWidget::sceneGraphError, this, &MainWindow::sceneGraphError);
#endif
  connect(ui->quickWidget, &QQuickWidget::statusChanged, this, &MainWindow::canvasLoaded);
  QUrl source("qrc:/src/windows/main.qml");
  ui->quickWidget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
  ui->quickWidget->setSource(source);
  ui->quickWidget->show();
  connect(ui->quickWidget, &CanvasWidget::enterCanvasWidget, [=]() {
    canvas_->setHoverMove(true);
    });
  connect(ui->quickWidget, &CanvasWidget::leaveCanvasWidget, [=]() {
    canvas_->setHoverMove(false);
  });
  connect(ui->quickWidget, &CanvasWidget::dropFile,[=](QPoint point, QString filename) {
    QFile file(filename);

    if (file.open(QFile::ReadOnly)) {
      // Update default file path
      QFileInfo file_info{filename};
      FilePathSettings::setDefaultFilePath(file_info.absoluteDir().absolutePath());

      QByteArray data = file.readAll();
      qInfo() << "File size:" << data.size();

      if (filename.toLower().endsWith(".bb")) {
        if ( ! handleUnsavedChange()) {
          return;
        }
        QDataStream stream(data);
        DocumentSerializer ds(stream);
        canvas_->setDocument(ds.deserializeDocument());
        canvas_->document().setCurrentFile(filename);
        canvas_->emitAllChanges();
        Q_EMIT canvas_->selectionsChanged(canvas_->document().selections());
        current_filename_ = QFileInfo(filename).baseName();
        setWindowFilePath(filename);
        setWindowTitle(current_filename_ + " - Swiftray");
      } else if (filename.toLower().endsWith(".svg") || filename.toLower().endsWith(".bvg")) {
        canvas_->loadSVG(filename);
        // canvas_->loadSVG(data);
        double scale = 10;//define by 3cm Ruler
        QPointF paste_shift(canvas_->document().getCanvasCoord(point));
        canvas_->transformControl().updateTransform(paste_shift.x(), paste_shift.y(), 
                                                    mainApp->getTransformR(), 
                                                    mainApp->getTransformW() * scale, mainApp->getTransformH() * scale);
      }  else if (filename.toLower().endsWith(".dxf")) {
        QTemporaryDir dir;
        QString temp_dxf_filepath = dir.isValid() ? dir.filePath("temp.dxf") : "temp.dxf";
        QFile src_file(filename);
        src_file.copy(temp_dxf_filepath);
        canvas_->loadDXF(temp_dxf_filepath);
        QFile::remove(temp_dxf_filepath);
        QPointF paste_shift(canvas_->document().getCanvasCoord(point));
        canvas_->transformControl().updateTransform(paste_shift.x(), paste_shift.y(), 
                                                    mainApp->getTransformR(), 
                                                    mainApp->getTransformW() * 10, mainApp->getTransformH() * 10);
      } else if (filename.toLower().endsWith(".pdf") || filename.toLower().endsWith(".ai")) {
        QTemporaryDir dir;
        Parser::PDF2SVG pdf_converter;
        QString sanitized_filepath = dir.isValid() ? dir.filePath("temp.pdf") : "temp.pdf";
        QString temp_svg_filepath = dir.isValid() ? dir.filePath("temp.svg") : "temp.svg";
        QFile src_file(filename);
        src_file.copy(sanitized_filepath);
        pdf_converter.convertPDFFile(sanitized_filepath, temp_svg_filepath);
        canvas_->loadSVG(temp_svg_filepath);
        pdf_converter.removeSVGFile(temp_svg_filepath);
        QFile::remove(sanitized_filepath);
        QPointF paste_shift(canvas_->document().getCanvasCoord(point));
        canvas_->transformControl().updateTransform(paste_shift.x(), paste_shift.y(), 
                                                    mainApp->getTransformR(), 
                                                    mainApp->getTransformW() * 10, mainApp->getTransformH() * 10);
      } else {
        importImage(filename);
        QPointF paste_shift(canvas_->document().getCanvasCoord(point));
        canvas_->transformControl().updateTransform(paste_shift.x(), paste_shift.y(), 
                                                    mainApp->getTransformR(), 
                                                    mainApp->getTransformW() * 10, mainApp->getTransformH() * 10);
      }
    }
  });
}

void MainWindow::loadStyles() {
  QFile file(isDarkMode() ?
             ":/resources/styles/swiftray-dark.qss" :
             ":/resources/styles/swiftray-light.qss");
  file.open(QFile::ReadOnly);
  QString styleSheet = QLatin1String(file.readAll());
  setStyleSheet(styleSheet);

  QList<QToolBar *> toolbars = {
       ui->toolBar,
       ui->toolBarAlign,
       ui->toolBarBool,
       ui->toolBarFile,
       ui->toolBarFlip,
       ui->toolBarGroup,
       ui->toolBarImage,
       ui->toolBarTask,
       ui->toolBarTransform,
       ui->toolBarVector,
  };
  for (QToolBar *toolbar : toolbars) {
    for (QAction *action : toolbar->actions()) {
      auto name = action->objectName().mid(6).toLower();
      action->setIcon(QIcon(
           (isDarkMode() ? ":/resources/images/dark/icon-" : ":/resources/images/icon-") + name
      ));
    }
  }
  ui->actionStart_2->setIcon(QIcon((isDarkMode() ? ":/resources/images/dark/icon-start.png" : ":/resources/images/icon-start.png")));
}


void MainWindow::initSparkle()
{
  software_update_init();
}
void MainWindow::checkForUpdates()
{
  software_update_check();
}

/**
 * @brief Check whether there is any unsaved change, and ask user to save
 * @return true if unsaved change is resolved
 *         false if unsaved change hasn't been resolved
 */
bool MainWindow::handleUnsavedChange() {
  bool handle_result = true;
  if (canvas_->document().currentFileModified()) {
    // some modifications have been performed on this document
    QMessageBox msgBox;
    msgBox.setText(tr("The document has been modified.\nDo you want to save your changes?"));
    msgBox.addButton(tr("Save"), QMessageBox::AcceptRole);
    msgBox.addButton(tr("Don't Save"), QMessageBox::DestructiveRole);
    msgBox.addButton(tr("Cancel"), QMessageBox::RejectRole);
    int ret = msgBox.exec();
    switch (ret) {
      case QMessageBox::AcceptRole:
          if(!canvas_->document().currentFile().isEmpty()) saveFile();
          else handle_result = saveAsFile();
          break;
      case QMessageBox::RejectRole:
          // Don't Save was clicked
          handle_result = true;
          break;
      case QMessageBox::DestructiveRole:
          // Cancel was clicked
          handle_result = false;
      default:
          // should never be reached
          handle_result = false;
    }
  }
  return handle_result;
}

/**
 * @brief When start button on the toolbar is pressed, 
 *        try to open job dashboard
 */
void MainWindow::actionStart() {
  // NOTE: unselect all to show the correct shape colors. Otherwise, the selected shapes will be in blue color.
  canvas_->document().execute(Commands::Select(&canvas_->document(), {}));

  // TODO: Restriction 1: Reject if active_machine not connected
  if (active_machine.getConnectionState() != Machine::ConnectionState::kConnected) {
    QMessageBox msgbox;
    msgbox.setText(tr("Serial Port Error"));
    msgbox.setInformativeText(tr("Please connect to serial port first"));
    msgbox.exec();
    return;
  }

  // Restriction 2: Reject if not job_executor exists in active_machine
  if (active_machine.getJobExecutor().isNull()) {
    return;
  }
  // Restriction 3: Reject if active job exists, but doesn't contain preview 
  if (active_machine.getJobExecutor()->getActiveJob()) {
    if (!active_machine.getJobExecutor()->getActiveJob()->withPreview()) {
      return;
    }
    // Re-open job dashboard for existing job
    //if (job_dashboard_) {
    //  job_dashboard_->deleteLater();
    //}
    job_dashboard_ = new JobDashboardDialog(
        active_machine.getJobExecutor()->getActiveJob()->getTotalRequiredTime(), 
        active_machine.getJobExecutor()->getActiveJob()->getPreview(), 
        this
    );
    // Attach to existing job
    job_dashboard_->attachJob(active_machine.getJobExecutor());
  } else {
    // If active job doesn't exist, Prepare (Generate) GCodes before launching job dashboard
    if (generateGcode() == false) {
      return;
    }
    try {
      auto gcode_list = gcode_panel_->getGCode().split('\n');
      // Estimate total required time
      auto progress_dialog = new QProgressDialog(
        tr("Estimating task time..."),  
        tr("Cancel"), 
        0, gcode_list.size() - 1, 
        this);
      auto timestamp_list = MachineJob::calcRequiredTime(gcode_list, progress_dialog);
      Timestamp total_required_time{0, 0};
      if (!timestamp_list.empty()) {
        total_required_time = timestamp_list.last();
      }
      // Prepare canvas scene pixmap
      QPixmap canvas_pixmap{
          static_cast<int>(canvas_->document().width()), 
          static_cast<int>(canvas_->document().height())
      };
      canvas_pixmap.fill(Qt::white);
      auto painter = std::make_unique<QPainter>(&canvas_pixmap);
      canvas_->document().paintUnselected(painter.get(), 2);
      canvas_->document().paintSelected(painter.get(), 2);
      //if (job_dashboard_) {
      //  job_dashboard_->deleteLater();
      //}
      job_dashboard_ = new JobDashboardDialog(total_required_time, canvas_pixmap, this);
    } catch (...) {
      // Terminated in required time estimation
      return;
    }
  }

  connect(job_dashboard_, &JobDashboardDialog::startBtnClicked, this, &MainWindow::onStartNewJob);
  connect(job_dashboard_, &JobDashboardDialog::pauseBtnClicked, this, &MainWindow::onPauseJob);
  connect(job_dashboard_, &JobDashboardDialog::resumeBtnClicked, this, &MainWindow::onResumeJob);
  connect(job_dashboard_, &JobDashboardDialog::stopBtnClicked, this, &MainWindow::onStopJob);
  connect(job_dashboard_, &JobDashboardDialog::jobStatusReport, this, &MainWindow::syncJobState);

  connect(job_dashboard_, &JobDashboardDialog::finished, this, &MainWindow::jobDashboardFinish);
  job_dashboard_->show();
  job_dashboard_->activateWindow();
  job_dashboard_->raise();
  job_dashboard_exist_ = true;
}

void MainWindow::actionFrame() {

  // Make sure active machine and job executor exist, 
  if (active_machine.getJobExecutor().isNull()) {
    return;
  }
  // Make sure port connected
  if (active_machine.getConnectionState() != Machine::ConnectionState::kConnected) {
    QMessageBox msgbox;
    msgbox.setText(tr("Serial Port Error"));
    msgbox.setInformativeText(tr("Please connect to serial port first"));
    msgbox.exec();
    return;
  }
  // Make sure no active job running
  if (active_machine.getJobExecutor()->getActiveJob()) {
    return;
  }

  // Generate gcode for framing
  QTransform move_translate = calculateTranslate();
  auto gen_outline_scanning_gcode = std::make_shared<DirtyAreaOutlineGenerator>(mainApp->getMachineParam(), mainApp->isRotaryMode());
  ToolpathExporter exporter(gen_outline_scanning_gcode.get(), 
      canvas_->document().settings().dpmm(),
      mainApp->getTravelSpeed(),
      end_point_,
      ToolpathExporter::PaddingType::kNoPadding,
      move_translate);
  exporter.setSortRule(mainApp->getPathSort());
  exporter.setWorkAreaSize(QRectF(0,0,canvas_->document().width() / 10, canvas_->document().height() / 10)); // TODO: Set machine work area in unit of mm
  exporter.convertStack(canvas_->document().layers(), mainApp->isHighSpeedMode(), mainApp->getStartWithHome());
  if (exporter.isExceedingBoundary()) {
    QMessageBox msgbox;
    msgbox.setText(tr("Warning"));
    msgbox.setInformativeText(tr("Some items aren't placed fully inside the working area."));
    msgbox.exec();
  }

  // Again, make sure active machine and job executor exist, 
  if (active_machine.getJobExecutor().isNull()) {
    return;
  }
  // Again, make sure port connected
  if (active_machine.getConnectionState() != Machine::ConnectionState::kConnected) {
    return;
  }
  // Again, make sure no active job running
  if (active_machine.getJobExecutor()->getActiveJob()) {
    return;
  }

  gen_outline_scanning_gcode->setTravelSpeed(mainApp->getTravelSpeed()*60);// mm/s to mm/min
  gen_outline_scanning_gcode->setLaserPower(mainApp->getFramingPower());
  // Create Framing Job and start
  if (true == active_machine.createFramingJob(
        QString::fromStdString(gen_outline_scanning_gcode->toString()).split("\n"))) 
  {
    gcode_panel_->attachJob(active_machine.getJobExecutor());
    active_machine.startJob();
  }
}

QPoint MainWindow::calculateJobOrigin() {
  //to get shape bounding
  QRect rect = canvas_->calculateShapeBoundary();
  int job_origin = mainApp->getJobOrigin();
  QPoint target_point;
  switch(job_origin) {
    case JobOrigin::NW:
      target_point = QPoint(rect.left(), rect.top());
      break;
    case JobOrigin::N:
      target_point = QPoint(rect.left() + rect.width()/2.0, rect.top());
      break;
    case JobOrigin::NE:
      target_point = QPoint(rect.left() + rect.width(), rect.top());
      break;
    case JobOrigin::E:
      target_point = QPoint(rect.left() + rect.width(), rect.top() + rect.height()/2.0);
      break;
    case JobOrigin::SE:
      target_point = QPoint(rect.left() + rect.width(), rect.top() + rect.height());
      break;
    case JobOrigin::S:
      target_point = QPoint(rect.left() + rect.width()/2.0, rect.top() + rect.height());
      break;
    case JobOrigin::SW:
      target_point = QPoint(rect.left(), rect.top() + rect.height());
      break;
    case JobOrigin::W:
      target_point = QPoint(rect.left(), rect.top() + rect.height()/2.0);
      break;
    case JobOrigin::CENTER:
      target_point = QPoint(rect.left() + rect.width()/2.0, rect.top() + rect.height()/2.0);
      break;
    default:
      break;
  }
  return target_point;
}

QTransform MainWindow::calculateTranslate() {
  int start_from = mainApp->getStartFrom();
  QTransform move_translate = QTransform();
  //to get shape bounding
  QPoint target_point = calculateJobOrigin();
  //unit??
  switch(start_from) {
    case StartFrom::AbsoluteCoords:
      end_point_ = QPointF(std::get<0>(active_machine.getCustomOrigin()), std::get<1>(active_machine.getCustomOrigin()));
      break;
    case StartFrom::CurrentPosition:
      move_translate.translate(std::get<0>(active_machine.getCurrentPosition()) * 10 - target_point.x(), 
                              std::get<1>(active_machine.getCurrentPosition()) * 10 - target_point.y());
      end_point_ = QPointF(std::get<0>(active_machine.getCurrentPosition()), std::get<1>(active_machine.getCurrentPosition()));
      break;
    case StartFrom::UserOrigin:
      move_translate.translate(std::get<0>(active_machine.getCustomOrigin()) * 10 - target_point.x(), 
                              std::get<1>(active_machine.getCustomOrigin()) * 10 - target_point.y());
      end_point_ = QPointF(std::get<0>(active_machine.getCustomOrigin()), std::get<1>(active_machine.getCustomOrigin()));
      break;
    default:
      break;
  }
  if(mainApp->isRotaryMode()) {
    int direction = 1;
    if(mainApp->isMirrorMode()) {
      direction = -1;
    }
    move_translate *= QTransform::fromScale(1,direction);
    //to move obj into canvas
    QRect rect = move_translate.mapRect(canvas_->calculateShapeBoundary());
    while(rect.top() < 0) {
      move_translate.translate(0,mainApp->getRotaryCircumference() * 10 * direction);
      rect = move_translate.mapRect(canvas_->calculateShapeBoundary());
    }
    move_translate *= QTransform::fromScale(1, mainApp->getRotaryScale());
  }
  return move_translate;
}

void MainWindow::showHighSpeedWarning() {
  if(mainApp->isHighSpeedMode()) {
    QMessageBox msgbox;
    msgbox.setText(tr("Warning"));
    msgbox.setInformativeText(tr("Please confirm that you are using the Lazervida machine."));
    msgbox.exec();
  }
}

void MainWindow::newFile() {
  if ( ! handleUnsavedChange()) {
    return;
  }

  qreal width = canvas_->document().width();
  qreal height = canvas_->document().height();
  canvas_->setDocument(new Document());
  canvas_->document().setCurrentFile("");
  canvas_->document().setWidth(width);
  canvas_->document().setHeight(height);
  canvas_->emitAllChanges();
  Q_EMIT canvas_->selectionsChanged(canvas_->document().selections());
  setWindowModified(false);
  setWindowTitle(tr("Untitled") + " - Swiftray");
  current_filename_ = tr("Untitled");
}

void MainWindow::onScalePlusClicked() {
  canvas_->setScaleWithCenter(qreal(qRound((canvas_->document().scale() + 0.0051)*100))/100);
}

void MainWindow::onScaleMinusClicked() {
  canvas_->setScaleWithCenter(qreal(qRound((canvas_->document().scale() - 0.0051)*100))/100);
}

/**
 * @brief Open Scene File or import SVG file
 */
void MainWindow::openFile() {
  if ( ! handleUnsavedChange()) {
    return;
  }
  QString default_open_dir = FilePathSettings::getDefaultFilePath();
  QString file_name = QFileDialog::getOpenFileName(this, "Open File", default_open_dir,
                                                   tr("Files (*.bb *.bvg *.svg *.png *.jpg *.jpeg *.bmp *.dxf *.pdf *.ai)"));

  if (!QFile::exists(file_name))
    return;

  QFile file(file_name);

  if (file.open(QFile::ReadOnly)) {
    // Update default file path
    QFileInfo file_info{file_name};
    FilePathSettings::setDefaultFilePath(file_info.absoluteDir().absolutePath());

    QByteArray data = file.readAll();
    qInfo() << "File size:" << data.size();

    if (file_name.toLower().endsWith(".bb")) {
      QDataStream stream(data);
      DocumentSerializer ds(stream);
      canvas_->setDocument(ds.deserializeDocument());
      canvas_->document().setCurrentFile(file_name);
      canvas_->emitAllChanges();
      Q_EMIT canvas_->selectionsChanged(canvas_->document().selections());
      current_filename_ = QFileInfo(file_name).baseName();
      setWindowFilePath(file_name);
      setWindowTitle(current_filename_ + " - Swiftray");
    } else if (file_name.toLower().endsWith(".svg") || file_name.toLower().endsWith(".bvg")) {
      canvas_->loadSVG(file_name);
      // canvas_->loadSVG(data);
    } else if (file_name.toLower().endsWith(".dxf")) {
      QTemporaryDir dir;
      QString temp_dxf_filepath = dir.isValid() ? dir.filePath("temp.dxf") : "temp.dxf";
      QFile src_file(file_name);
      src_file.copy(temp_dxf_filepath);
      canvas_->loadDXF(temp_dxf_filepath);
      QFile::remove(temp_dxf_filepath);
    } else if (file_name.toLower().endsWith(".pdf") || file_name.toLower().endsWith(".ai")) {
      QTemporaryDir dir;
      Parser::PDF2SVG pdf_converter;
      QString sanitized_filepath = dir.isValid() ? dir.filePath("temp.pdf") : "temp.pdf";
      QString temp_svg_filepath = dir.isValid() ? dir.filePath("temp.svg") : "temp.svg";
      QFile src_file(file_name);
      src_file.copy(sanitized_filepath);
      pdf_converter.convertPDFFile(sanitized_filepath, temp_svg_filepath);
      canvas_->loadSVG(temp_svg_filepath);
      //QFile svg_file(temp_svg_filepath);
      //if (svg_file.open(QFile::ReadOnly)) {
      //  QByteArray data = svg_file.readAll();
      //  canvas_->loadSVG(data);
      //}
      pdf_converter.removeSVGFile(temp_svg_filepath);
      QFile::remove(sanitized_filepath);
    } else {
      importImage(file_name);
    }
  }
}

void MainWindow::openExampleOfSwiftray() {
  QString file_name = ":/resources/example/Example-of-swiftray.bb";
  if (!QFile::exists(file_name)) 
    return;
  QFile file(file_name);
  if (file.open(QFile::ReadOnly)) {
    // Update default file path
    QFileInfo file_info{file_name};
    setWindowFilePath(FilePathSettings::getDefaultFilePath());
    current_filename_ = QFileInfo(file_name).baseName();
    QByteArray data = file.readAll();
    QDataStream stream(data);
    DocumentSerializer ds(stream);
    canvas_->setDocument(ds.deserializeDocument());
    canvas_->document().setCurrentFile(file_name);
    canvas_->emitAllChanges();
    Q_EMIT canvas_->selectionsChanged(canvas_->document().selections());
  }
}

void MainWindow::openMaterialCuttingTest() {
  QString file_name = ":/resources/example/Material-Cutting-Test.bb";
  if (!QFile::exists(file_name)) 
    return;
  QFile file(file_name);
  if (file.open(QFile::ReadOnly)) {
    // Update default file path
    QFileInfo file_info{file_name};
    setWindowFilePath(FilePathSettings::getDefaultFilePath());
    current_filename_ = QFileInfo(file_name).baseName();
    QByteArray data = file.readAll();
    QDataStream stream(data);
    DocumentSerializer ds(stream);
    canvas_->setDocument(ds.deserializeDocument());
    canvas_->document().setCurrentFile(file_name);
    canvas_->emitAllChanges();
    Q_EMIT canvas_->selectionsChanged(canvas_->document().selections());
  }
}

void MainWindow::openMaterialEngravingTest() {
  QString file_name = ":/resources/example/Material-Engraving-Test.bb";
  if (!QFile::exists(file_name)) 
    return;
  QFile file(file_name);
  if (file.open(QFile::ReadOnly)) {
    // Update default file path
    QFileInfo file_info{file_name};
    setWindowFilePath(FilePathSettings::getDefaultFilePath());
    current_filename_ = QFileInfo(file_name).baseName();
    QByteArray data = file.readAll();
    QDataStream stream(data);
    DocumentSerializer ds(stream);
    canvas_->setDocument(ds.deserializeDocument());
    canvas_->document().setCurrentFile(file_name);
    canvas_->emitAllChanges();
    Q_EMIT canvas_->selectionsChanged(canvas_->document().selections());
  }
}

/**
 * @brief Save the document with the origin filename
 *        If the document has never been saved, force a new save as
 */
void  MainWindow::saveFile() {
  qInfo() << canvas_->document().currentFile();
  if (canvas_->document().currentFile().isEmpty()) {
    saveAsFile();
    return;
  }

  canvas_->exitCurrentMode();
  canvas_->document().setSelection(nullptr);
  QFile file(canvas_->document().currentFile());
  if (file.open(QFile::ReadWrite)) {
    QDataStream stream(&file);
    canvas_->save(stream);
    file.close();
    canvas_->document().setCurrentFile(canvas_->document().currentFile());
    qInfo() << "Saved";
  }
}

/**
 * @brief Save the document with a new filename
 */
bool MainWindow::saveAsFile() {
  bool result = false;
  QString default_save_dir = FilePathSettings::getDefaultFilePath();

  QString filter = tr("Scene File (*.bb)");
  QString file_name = QFileDialog::getSaveFileName(this,
                                                   tr("Save Image"),
                                                   default_save_dir,
                                                   filter, &filter);

  //QString file_name = QFileDialog::getSaveFileName(this, "Save Image", ".", tr("Scene File (*.bb)"));
  QFile file(file_name);
  canvas_->exitCurrentMode();
  canvas_->document().setSelection(nullptr);
  if (file.open(QFile::ReadWrite)) {
    // Update default file path
    QFileInfo file_info{file_name};
    setWindowFilePath(file_name);
    current_filename_ = QFileInfo(file_name).baseName();
    FilePathSettings::setDefaultFilePath(file_info.absoluteDir().absolutePath());

    QDataStream stream(&file);
    canvas_->save(stream);
    file.close();
    canvas_->document().setCurrentFile(file_name);
    qInfo() << "Saved";
    result = true;
  }
  return result;
}

void MainWindow::importImage(QString file_name) {
  QImage image;

  if (image.load(file_name)) {
    qInfo() << "File size:" << image.size();
    canvas_->importImage(image);
  }
}

void MainWindow::openImageFile() {
  canvas_->exitCurrentMode();
  if (canvas_->document().activeLayer()->isLocked()) {
    Q_EMIT canvas_->modeChanged();
    return;
  }

#ifdef Q_OS_IOS
  // TODO (Possible leak here?)
  ImagePicker *p = new ImagePicker();
  connect(p, &ImagePicker::imageSelected, this, &MainWindow::imageSelected);
  p->show();
  return;
#endif

  QString default_open_dir = FilePathSettings::getDefaultFilePath();
  QString file_name = QFileDialog::getOpenFileName(this,
                                                   "Open Image",
                                                   default_open_dir,
                                                   tr("Image Files (*.png *.jpg *.jpeg *.bvg *.svg *.bmp *.dxf *.pdf *.ai)"));

  if (!QFile::exists(file_name))
    return;

  QFileInfo file_info{file_name};
  FilePathSettings::setDefaultFilePath(file_info.absoluteDir().absolutePath());

  if (file_name.toLower().endsWith(".svg") || file_name.toLower().endsWith(".bvg")) {
    canvas_->loadSVG(file_name);
  } else if (file_name.toLower().endsWith(".dxf")) {
    QTemporaryDir dir;
    QString temp_dxf_filepath = dir.isValid() ? dir.filePath("temp.dxf") : "temp.dxf";
    QFile src_file(file_name);
    src_file.copy(temp_dxf_filepath);
    canvas_->loadDXF(temp_dxf_filepath);
    QFile::remove(temp_dxf_filepath);
  } else if (file_name.toLower().endsWith(".pdf") || file_name.toLower().endsWith(".ai")) {
    QTemporaryDir dir;
    Parser::PDF2SVG pdf_converter;
    QString sanitized_filepath = dir.isValid() ? dir.filePath("temp.pdf") : "temp.pdf";
    QString temp_svg_filepath = dir.isValid() ? dir.filePath("temp.svg") : "temp.svg";
    QFile src_file(file_name);
    src_file.copy(sanitized_filepath);
    pdf_converter.convertPDFFile(sanitized_filepath, temp_svg_filepath);
    canvas_->loadSVG(temp_svg_filepath);
    //QFile svg_file(temp_svg_filepath);
    //if (svg_file.open(QFile::ReadOnly)) {
    //  QByteArray data = svg_file.readAll();
    //  canvas_->loadSVG(data);
    //}
    pdf_converter.removeSVGFile(temp_svg_filepath);
    QFile::remove(sanitized_filepath);
  } else {
    importImage(file_name);
  }

  canvas_->setMode(Canvas::Mode::Selecting);
}

void MainWindow::replaceImage() {
  QString default_open_dir = FilePathSettings::getDefaultFilePath();
  QString file_name = QFileDialog::getOpenFileName(this,
                                                   "Open Image",
                                                   default_open_dir,
                                                   tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));
  QImage new_image;
  if (QFile::exists(file_name) && new_image.load(file_name)) {
    canvas_->replaceImage(new_image);

    // Update default file path
    FilePathSettings::setDefaultFilePath(QFileInfo{file_name}.absoluteDir().absolutePath());
  }
}

void MainWindow::imageSelected(const QImage image) {
  QImage my_image = image;
  canvas_->importImage(my_image);
}

void MainWindow::exportGCodeFile() {
  auto gen_gcode = std::make_shared<GCodeGenerator>(mainApp->getMachineParam(), mainApp->isRotaryMode());
  QProgressDialog progress_dialog(tr("Generating GCode..."),
                                   tr("Cancel"),
                                   0,
                                   100, this);
  progress_dialog.setWindowModality(Qt::WindowModal);
  progress_dialog.show();
  progress_dialog.activateWindow();
  progress_dialog.raise();

  QTransform move_translate = calculateTranslate();
  ToolpathExporter exporter(gen_gcode.get(),
      canvas_->document().settings().dpmm(), 
      mainApp->getTravelSpeed(),
      end_point_,
      ToolpathExporter::PaddingType::kFixedPadding,
      move_translate);
  exporter.setSortRule(mainApp->getPathSort());
  exporter.setWorkAreaSize(QRectF(0,0,canvas_->document().width() / 10, canvas_->document().height() / 10)); // TODO: Set machine work area in unit of mm
  if ( true != exporter.convertStack(canvas_->document().layers(), mainApp->isHighSpeedMode(), 
                                    mainApp->getStartWithHome(), &progress_dialog)) {
    return; // canceled
  }

  QString default_save_dir = FilePathSettings::getDefaultFilePath();

  QString filter = tr("GCode Files (*.gcode)");
  QString file_name = QFileDialog::getSaveFileName(this,
                                                   tr("Save GCode"),
                                                   default_save_dir,
                                                   filter, &filter);
  if (file_name.isEmpty()) {
    return;
  }
  QFile file(file_name);
  if (file.open(QFile::WriteOnly)) {
    QTextStream stream(&file);
    stream << QString::fromStdString(gen_gcode->toString()).toUtf8();
    file.close();

    // update default save dir
    QFileInfo file_info{file_name};
    FilePathSettings::setDefaultFilePath(file_info.absoluteDir().absolutePath());
  }
}

void MainWindow::importGCodeFile() {
  if ( ! handleUnsavedChange()) {
    return;
  }
  QString default_open_dir = FilePathSettings::getDefaultFilePath();
  QString file_name = QFileDialog::getOpenFileName(this, "Open GCode", default_open_dir,
                                                   tr("GCdoe Files (*.gc, *.gcode)"));

  if (!QFile::exists(file_name))
    return;

  QFile file(file_name);

  if (file.open(QFile::ReadOnly)) {
    // Update default file path
    QFileInfo file_info{file_name};
    FilePathSettings::setDefaultFilePath(file_info.absoluteDir().absolutePath());

    QByteArray data = file.readAll();
    qInfo() << "File size:" << data.size();
    QTextStream stream(data);
    gcode_panel_->setGCode(stream.readAll());
  }
}

void MainWindow::canvasLoaded(QQuickWidget::Status status) {
  if (status == QQuickWidget::Error) {
    const auto widgetErrors = this->ui->quickWidget->errors();

    for (const QQmlError &error : widgetErrors)
      qInfo() << error.toString();

    Q_ASSERT_X(false, "QQuickWidget Initialization", "QQuickWidget failed to initialize");
  }

  canvas_ = ui->quickWidget->rootObject()->findChildren<Canvas *>().first();
  canvas_->setWidget(ui->quickWidget);
  // TODO (Chanage the owner of text_box_ to mainwindow)
  canvas_->text_input_ = new CanvasTextEdit(this);
  canvas_->text_input_->setGeometry(0, 0, 0, 0);
  canvas_->text_input_->setStyleSheet("border:0");
}

bool MainWindow::event(QEvent *e) {
  switch (e->type()) {
    case QEvent::CursorChange:
    case QEvent::UpdateRequest:
      break;

    case QEvent::NativeGesture:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
      canvas_->event(e);
      return true;

    default:
      break;
  }

  return QMainWindow::event(e);
}

void MainWindow::sceneGraphError(QQuickWindow::SceneGraphError, const QString &message) {
  // statusBar()->showMessage(message);
}

MainWindow::~MainWindow() {
  software_update_cleanup();
  serial_port.close();
  delete ui;
}

void MainWindow::updateMode() {
  for (QAction *action : ui->toolBar->actions()) {
    action->setChecked(false);
  }

  const std::map<Canvas::Mode, QAction *> actionMap = {
       {Canvas::Mode::Selecting,      ui->actionSelect},
       {Canvas::Mode::MultiSelecting, ui->actionSelect},
       {Canvas::Mode::LineDrawing,    ui->actionLine},
       {Canvas::Mode::RectDrawing,    ui->actionRect},
       {Canvas::Mode::OvalDrawing,    ui->actionOval},
       {Canvas::Mode::PathDrawing,    ui->actionPath},
       {Canvas::Mode::TextDrawing,    ui->actionText},
       {Canvas::Mode::PolygonDrawing, ui->actionPolygon},
  };

  auto actionIter = actionMap.find(canvas_->mode());
  if (actionIter != actionMap.end()) actionIter->second->setChecked(true);
}

void MainWindow::updateScale() {
  scale_block_->setText(QString::number(canvas_->document().scale() * 100, 'f', 1)+"%");
}

void MainWindow::loadWidgets() {
  assert(canvas_ != nullptr);
  // TODO (Use event to decouple circular dependency with Mainwindow)
  transform_panel_ = new TransformPanel(ui->objectParamDock, this);
  layer_panel_ = new LayerPanel(ui->layerDockContents, this);
  gcode_panel_ = new GCodePanel(ui->serialPortDock, this);
  font_panel_ = new FontPanel(ui->fontDock, isDarkMode());
  image_panel_ = new ImagePanel(ui->imageDock, this);
  doc_panel_ = new DocPanel(ui->documentDock, this);
  jogging_panel_ = new JoggingPanel(ui->joggingDock, this);
  laser_panel_ = new LaserPanel(ui->laserDock, isDarkMode());
  machine_manager_ = new MachineManager(this, mainApp->getMachineIndex());
  preferences_window_ = new PreferencesWindow(this);
  about_window_ = new AboutWindow(this);
  welcome_dialog_ = new WelcomeDialog(this);
  privacy_window_ = new PrivacyWindow(this);
  RotarySetup::RotarySetting rotary_setting;
  rotary_setting.rotary_mode = mainApp->isRotaryMode();
  rotary_setting.mirror_mode = mainApp->isMirrorMode();
  rotary_setting.circumference = mainApp->getRotaryCircumference();
  rotary_setting.rotary_axis = mainApp->getRotaryAxis();
  rotary_setting.rotary_index = mainApp->getRotaryIndex();
  rotary_setup_ = new RotarySetup(rotary_setting, this);
  ui->joggingDock->setWidget(jogging_panel_);
  ui->objectParamDock->setWidget(transform_panel_);
  ui->serialPortDock->setWidget(gcode_panel_);
  ui->fontDock->setWidget(font_panel_);
  ui->imageDock->setWidget(image_panel_);
  ui->layerDock->setWidget(layer_panel_);
  ui->documentDock->setWidget(doc_panel_);
  ui->laserDock->setWidget(laser_panel_);
#ifdef Q_OS_IOS
  ui->serialPortDock->setVisible(false);
#endif

  //Initial panel setting by MainApplication
  font_panel_->setFontFamily(mainApp->getFont().family());
  font_panel_->setPointSize(mainApp->getFont().pointSize());
  font_panel_->setLetterSpacing(mainApp->getFont().letterSpacing());
  font_panel_->setBold(mainApp->getFont().bold());
  font_panel_->setItalic(mainApp->getFont().italic());
  font_panel_->setUnderline(mainApp->getFont().underline());
  font_panel_->setLineHeight(mainApp->getFontLineHeight());
  transform_panel_->setScaleLock(mainApp->isShapeScaleLocked());
  transform_panel_->setShapeReference(mainApp->getShapeReference());
  image_panel_->setImageGradient(mainApp->isImageGradient());
  image_panel_->setImageThreshold(mainApp->getImageThreshold());
  image_panel_->changeImageEnable(false);
}

void MainWindow::registerEvents() {
  connect(this, &MainWindow::windowWasShown, this, &MainWindow::initSparkle,
          Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));

  // Monitor canvas events
  connect(canvas_, &Canvas::modeChanged, this, &MainWindow::updateMode);
  connect(canvas_, &Canvas::scaleChanged, this, &MainWindow::updateScale);
  connect(canvas_, &Canvas::fileModifiedChange, this, &MainWindow::updateTitle);
  connect(canvas_, &Canvas::canvasContextMenuOpened, this, &MainWindow::showCanvasPopMenu);
  connect(canvas_, &Canvas::transformChanged, mainApp, &MainApplication::getSelectShapeTransform);
  connect(canvas_, &Canvas::selectionsChanged, mainApp, &MainApplication::getSelectShapeChange);
  // Monitor UI events
  connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newFile);
  connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
  connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFile);
  connect(ui->actionSave_As, &QAction::triggered, this, &MainWindow::saveAsFile);
  connect(ui->actionExampleOfSwiftray, &QAction::triggered, this, &MainWindow::openExampleOfSwiftray);
  connect(ui->actionMaterialCuttingTest, &QAction::triggered, this, &MainWindow::openMaterialCuttingTest);
  connect(ui->actionMaterialEngravingTest, &QAction::triggered, this, &MainWindow::openMaterialEngravingTest);
  connect(ui->actionClose, &QAction::triggered, this, &MainWindow::close);
  connect(ui->actionCut, &QAction::triggered, canvas_, &Canvas::editCut);
  connect(ui->actionCopy, &QAction::triggered, canvas_, &Canvas::editCopy);
  connect(ui->actionPaste, &QAction::triggered, [=]() {canvas_->editPaste();});
  connect(ui->actionUndo, &QAction::triggered, canvas_, &Canvas::editUndo);
  connect(ui->actionRedo, &QAction::triggered, canvas_, &Canvas::editRedo);
  connect(ui->actionSelect_All, &QAction::triggered, canvas_, &Canvas::editSelectAll);
  connect(ui->actionClear, &QAction::triggered, this, [=]() {
    if(handleUnsavedChange() ) {
      canvas_->editClear();
    }
  });
  connect(ui->actionGroup, &QAction::triggered, canvas_, &Canvas::editGroup);
  connect(ui->actionUngroup, &QAction::triggered, canvas_, &Canvas::editUngroup);
  connect(ui->actionSelect, &QAction::triggered, canvas_, &Canvas::exitCurrentMode);
  connect(ui->actionRect, &QAction::triggered, canvas_, &Canvas::editDrawRect);
  connect(ui->actionPolygon, &QAction::triggered, canvas_, &Canvas::editDrawPolygon);
  connect(ui->actionOval, &QAction::triggered, canvas_, &Canvas::editDrawOval);
  connect(ui->actionLine, &QAction::triggered, canvas_, &Canvas::editDrawLine);
  connect(ui->actionPath, &QAction::triggered, canvas_, &Canvas::editDrawPath);
  connect(ui->actionText, &QAction::triggered, canvas_, &Canvas::editDrawText);
  connect(ui->actionText, &QAction::triggered, [=]() {
    Q_EMIT mainApp->changeFontEnable(true);
  });
  connect(ui->actionPhoto, &QAction::triggered, this, &MainWindow::openImageFile);
  connect(ui->actionMarket, &QAction::triggered, [=]() {
    QDesktopServices::openUrl(QUrl("https://dmkt.io/"));
  });
  connect(ui->actionUnion, &QAction::triggered, canvas_, &Canvas::editUnion);
  connect(ui->actionSubtract, &QAction::triggered, canvas_, &Canvas::editSubtract);
  connect(ui->actionIntersect, &QAction::triggered, canvas_, &Canvas::editIntersect);
  connect(ui->actionDiff, &QAction::triggered, canvas_, &Canvas::editDifference);
  connect(ui->actionGroup, &QAction::triggered, canvas_, &Canvas::editGroup);
  connect(ui->actionGroupMenu, &QAction::triggered, canvas_, &Canvas::editGroup);
  connect(ui->actionUngroup, &QAction::triggered, canvas_, &Canvas::editUngroup);
  connect(ui->actionUngroupMenu, &QAction::triggered, canvas_, &Canvas::editUngroup);
  connect(ui->actionHFlip, &QAction::triggered, canvas_, &Canvas::editHFlip);
  connect(ui->actionVFlip, &QAction::triggered, canvas_, &Canvas::editVFlip);
  connect(ui->actionAlignVTop, &QAction::triggered, canvas_, &Canvas::editAlignVTop);
  connect(ui->actionAlignVCenter, &QAction::triggered, canvas_, &Canvas::editAlignVCenter);
  connect(ui->actionAlignVBottom, &QAction::triggered, canvas_, &Canvas::editAlignVBottom);
  connect(ui->actionAlignHLeft, &QAction::triggered, canvas_, &Canvas::editAlignHLeft);
  connect(ui->actionAlignHCenter, &QAction::triggered, canvas_, &Canvas::editAlignHCenter);
  connect(ui->actionAlignHRight, &QAction::triggered, canvas_, &Canvas::editAlignHRight);
  connect(ui->actionPreferences, &QAction::triggered, [=]() {
    preferences_window_->setSpeedMode(mainApp->isHighSpeedMode());
    preferences_window_->setCanvasQuality(mainApp->getCanvasQuality());
    preferences_window_->setPathSort(mainApp->getPathSort());
    preferences_window_->show();
    preferences_window_->activateWindow();
    preferences_window_->raise();
  });
  connect(ui->actionAbout, &QAction::triggered, [=]() {
    about_window_->show();
    about_window_->activateWindow();
    about_window_->raise();
  });
#ifdef HAVE_SOFTWARE_UPDATE
  connect(ui->actionCheckForUpdates, &QAction::triggered, this, &MainWindow::checkForUpdates);
#endif

  connect(ui->actionMachineSettings, &QAction::triggered, [=]() {
    doc_panel_->setMachineSelectLock(false);
    machine_manager_->setMachineIndex(mainApp->getMachineIndex());
    machine_manager_->show();
    machine_manager_->activateWindow();
    machine_manager_->raise();
    if(machine_manager_->exec() != 1) {
      Q_EMIT mainApp->editMachineIndex(mainApp->getMachineIndex());
    }
    doc_panel_->setMachineSelectLock(true);
  });
  connect(ui->actionRotarySetup, &QAction::triggered, [=]() {
    rotary_setup_->setRotaryIndex(mainApp->getRotaryIndex());
    rotary_setup_->show();
    rotary_setup_->activateWindow();
    rotary_setup_->raise();
    if(rotary_setup_->exec() != 1) {
    }
  });
  connect(ui->actionPathOffset, &QAction::triggered, canvas_, &Canvas::genPathOffset);
  connect(ui->actionTrace, &QAction::triggered, canvas_, &Canvas::genImageTrace);
  connect(ui->actionInvert, &QAction::triggered, canvas_, &Canvas::invertImage);
  connect(ui->actionSharpen, &QAction::triggered, canvas_, &Canvas::sharpenImage);
  connect(ui->actionReplace_with, &QAction::triggered, this, &MainWindow::replaceImage);
  connect(ui->actionJogging, &QAction::triggered, this, &MainWindow::showJoggingPanel);
  connect(ui->actionCrop, &QAction::triggered, canvas_, &Canvas::cropImage);
  connect(ui->actionStart, &QAction::triggered, this, &MainWindow::actionStart);
  connect(ui->actionStart_2, &QAction::triggered, this, &MainWindow::actionStart);
  connect(ui->actionFrame, &QAction::triggered, this, &MainWindow::actionFrame);

  connect(ui->actionSaveClassic, &QAction::triggered, [=]() {
      QSettings settings(QDir::currentPath() + "/classicUI.ini", QSettings::IniFormat);
      settings.setValue("window/windowState", saveState());
  });
  connect(ui->actionSaveEssential, &QAction::triggered, [=]() {
      QSettings settings(QDir::currentPath() + "/essentialUI.ini", QSettings::IniFormat);
      settings.setValue("window/windowState", saveState());
  });
  connect(ui->actionLoadClassic, &QAction::triggered, [=]() {
      QSettings settings(":/classicUI.ini", QSettings::IniFormat);
      restoreState(settings.value("window/windowState").toByteArray());
  });
  connect(ui->actionLoadEssential, &QAction::triggered, [=]() {
      QSettings settings(":/essentialUI.ini", QSettings::IniFormat);
      restoreState(settings.value("window/windowState").toByteArray());
  });
  // Complex callbacks
  connect(welcome_dialog_, &WelcomeDialog::addNewMachine, [=](MachineSettings::MachineParam new_param) {
    mainApp->addMachine(new_param);
  });
  connect(welcome_dialog_, &WelcomeDialog::finished, [=](int result) {
    privacy_window_->show();
    privacy_window_->activateWindow();
    privacy_window_->raise();
  });
  connect(privacy_window_, &PrivacyWindow::finished, [=](int result) {
    showHighSpeedWarning();//to show warning window
  });
  connect(ui->actionExportGcode, &QAction::triggered, this, &MainWindow::exportGCodeFile);
  connect(ui->actionPreview, &QAction::triggered, this, &MainWindow::genPreviewWindow);
  connect(canvas_, &Canvas::cursorChanged, [=](Qt::CursorShape cursor) {
    if (cursor == Qt::ArrowCursor) {
      unsetCursor();
    } else {
      setCursor(cursor);
    }
  });
  connect(canvas_, &Canvas::syncJobOrigin, [=]() {
    canvas_->setJobOrigin(calculateJobOrigin());
  });

  connect(preferences_window_, &PreferencesWindow::speedModeChanged, [=](bool is_high_speed) {
    mainApp->updateMachineHighSpeedMode(is_high_speed);
  });
  connect(preferences_window_, &PreferencesWindow::privacyUpdate, [=](bool enable_upload) {
    mainApp->updateUploadEnable(enable_upload);
  });
  connect(preferences_window_, &PreferencesWindow::canvasQualityUpdate, [=](int canvas_quality) {
    mainApp->updateCanvasQuality(canvas_quality);
  });
  connect(preferences_window_, &PreferencesWindow::pathSortUpdate, [=](int path_sort) {
    mainApp->updatePathSort(path_sort);
  });
  connect(privacy_window_, &PrivacyWindow::privacyUpdate, [=](bool enable_upload) {
    mainApp->updateUploadEnable(enable_upload);
  });
  connect(mainApp, &MainApplication::editUploadEnable, [=](bool is_enable) {
#ifdef ENABLE_SENTRY
    if(is_enable) {
      sentry_user_consent_give();
    }
    else {
      sentry_user_consent_revoke();
    }
#endif
    preferences_window_->setUpload(is_enable);
  });
  //panel
  connect(ui->actionFontPanel, &QAction::triggered, [=]() {
    if(ui->fontDock->isHidden()) {
      ui->fontDock->show();
    }
    else {
      ui->fontDock->hide();
    }
  });
  connect(ui->actionObjectPanel, &QAction::triggered, [=]() {
    if(ui->objectParamDock->isHidden()) {
      ui->objectParamDock->show();
    }
    else {
      ui->objectParamDock->hide();
    }
  });
  connect(ui->actionLayerPanel, &QAction::triggered, [=]() {
    if(ui->layerDock->isHidden()) {
      ui->layerDock->show();
    }
    else {
      ui->layerDock->hide();
    }
  });
  connect(ui->actionGCodeViewerPanel, &QAction::triggered, [=]() {
    if(ui->serialPortDock->isHidden()) {
      ui->serialPortDock->show();
    }
    else {
      ui->serialPortDock->hide();
    }
  });
  connect(ui->actionImagePanel, &QAction::triggered, [=]() {
    if(ui->imageDock->isHidden()) {
      ui->imageDock->show();
    }
    else {
      ui->imageDock->hide();
    }
  });
  connect(ui->actionDocumentPanel, &QAction::triggered, [=]() {
    if(ui->documentDock->isHidden()) {
      ui->documentDock->show();
    }
    else {
      ui->documentDock->hide();
    }
  });
  connect(ui->actionJoggingPanel, &QAction::triggered, [=]() {
    if(ui->joggingDock->isHidden()) {
      ui->joggingDock->show();
    }
    else {
      ui->joggingDock->hide();
    }
  });
  connect(ui->actionLaser, &QAction::triggered, [=]() {
    if(ui->laserDock->isHidden()) {
      ui->laserDock->show();
    }
    else {
      ui->laserDock->hide();
    }
  });
  //toolbar
  connect(ui->actionAlign, &QAction::triggered, [=]() {
    if(ui->toolBarAlign->isHidden()) {
      ui->toolBarAlign->show();
    }
    else {
      ui->toolBarAlign->hide();
    }
  });
  connect(ui->actionBoolean, &QAction::triggered, [=]() {
    if(ui->toolBarBool->isHidden()) {
      ui->toolBarBool->show();
    }
    else {
      ui->toolBarBool->hide();
    }
  });
  connect(ui->actionConnection, &QAction::triggered, [=]() {
    if(ui->toolBarConnection->isHidden()) {
      ui->toolBarConnection->show();
    }
    else {
      ui->toolBarConnection->hide();
    }
  });
  connect(ui->actionFlip, &QAction::triggered, [=]() {
    if(ui->toolBarFlip->isHidden()) {
      ui->toolBarFlip->show();
    }
    else {
      ui->toolBarFlip->hide();
    }
  });
  connect(ui->actionFont, &QAction::triggered, [=]() {
    if(ui->toolBarFont->isHidden()) {
      ui->toolBarFont->show();
    }
    else {
      ui->toolBarFont->hide();
    }
  });
  connect(ui->actionFunctional, &QAction::triggered, [=]() {
    if(ui->toolBar->isHidden()) {
      ui->toolBar->show();
    }
    else {
      ui->toolBar->hide();
    }
  });
  connect(ui->actionGroup_2, &QAction::triggered, [=]() {
    if(ui->toolBarGroup->isHidden()) {
      ui->toolBarGroup->show();
    }
    else {
      ui->toolBarGroup->hide();
    }
  });
  connect(ui->actionImage, &QAction::triggered, [=]() {
    if(ui->toolBarImage->isHidden()) {
      ui->toolBarImage->show();
    }
    else {
      ui->toolBarImage->hide();
    }
  });
  connect(ui->actionProject, &QAction::triggered, [=]() {
    if(ui->toolBarFile->isHidden()) {
      ui->toolBarFile->show();
    }
    else {
      ui->toolBarFile->hide();
    }
  });
  connect(ui->actionTask, &QAction::triggered, [=]() {
    if(ui->toolBarTask->isHidden()) {
      ui->toolBarTask->show();
    }
    else {
      ui->toolBarTask->hide();
    }
  });
  connect(ui->actionTransform, &QAction::triggered, [=]() {
    if(ui->toolBarTransform->isHidden()) {
      ui->toolBarTransform->show();
    }
    else {
      ui->toolBarTransform->hide();
    }
  });
  connect(ui->actionVector, &QAction::triggered, [=]() {
    if(ui->toolBarVector->isHidden()) {
      ui->toolBarVector->show();
    }
    else {
      ui->toolBarVector->hide();
    }
  });

  connect(ui->actionConsole, &QAction::triggered, [=]() {
    if (!console_dialog_.isNull()) {
      console_dialog_.clear();
    }
    console_dialog_ = QSharedPointer<ConsoleDialog>::create(this);
    if (active_machine.getConnectionState() != Machine::ConnectionState::kDisconnected) {
      if (!console_dialog_.isNull()) {
        connect(&active_machine, &Machine::logSent, console_dialog_.data(), &ConsoleDialog::appendLogSent);
        connect(&active_machine, &Machine::logRcvd, console_dialog_.data(), &ConsoleDialog::appendLogRcvd);
      }
    }
    // Port connected but hasn't responded any meaningful response
    console_dialog_->show();
    console_dialog_->activateWindow();
    console_dialog_->raise();
  });

  //about transform
  connect(transform_panel_, &TransformPanel::editShapeTransformX, mainApp, &MainApplication::updateShapeTransformX);
  connect(transform_panel_, &TransformPanel::editShapeTransformY, mainApp, &MainApplication::updateShapeTransformY);
  connect(transform_panel_, &TransformPanel::editShapeTransformR, mainApp, &MainApplication::updateShapeTransformR);
  connect(transform_panel_, &TransformPanel::editShapeTransformW, mainApp, &MainApplication::updateShapeTransformW);
  connect(transform_panel_, &TransformPanel::editShapeTransformH, mainApp, &MainApplication::updateShapeTransformH);
  connect(transform_panel_, &TransformPanel::scaleLockToggled, mainApp, &MainApplication::updateShapeScaleLock);
  connect(transform_panel_, &TransformPanel::editShapeReference, mainApp, &MainApplication::updateShapeReference);
  connect(mainApp, &MainApplication::selectAllGeometry, [=](bool state) {
    ui->actionPathOffset->setEnabled(state);
  });
  connect(mainApp, &MainApplication::selectAllGroup, [=](bool state) {
    ungroupAction_->setEnabled(state);
    ui->actionUngroup->setEnabled(state);
    ui->actionUngroupMenu->setEnabled(state);
  });
  connect(mainApp, &MainApplication::changeUnionEnable, [=](bool state) {
    ui->actionUnion->setEnabled(state); // Union can be done with the shape itself if it contains sub polygons
  });
  connect(mainApp, &MainApplication::selectPairPath, [=](bool state) {
    ui->actionSubtract->setEnabled(state);
    ui->actionDiff->setEnabled(state);
    ui->actionIntersect->setEnabled(state);
  });
  connect(mainApp, &MainApplication::selectGroupEnable, [=](bool state) {
    groupAction_->setEnabled(state);
    ui->actionGroup->setEnabled(state);
    ui->actionGroupMenu->setEnabled(state);
    ui->actionAlignVTop->setEnabled(state);
    ui->actionAlignVCenter->setEnabled(state);
    ui->actionAlignVBottom->setEnabled(state);
    ui->actionAlignHLeft->setEnabled(state);
    ui->actionAlignHCenter->setEnabled(state);
    ui->actionAlignHRight->setEnabled(state);
  });
  connect(mainApp, &MainApplication::editShapeReference, [=](int reference_origin) {
    transform_panel_->setShapeReference(reference_origin);
    canvas_->setShapeReference(reference_origin);
  });

  //about font
  connect(font_panel_, &FontPanel::editShapeFontFamily, mainApp, &MainApplication::updateShapeFontFamily);
  connect(font_panel_, &FontPanel::editShapeFontPointSize, mainApp, &MainApplication::editShapeFontPointSize);
  connect(font_panel_, &FontPanel::editShapeLetterSpacing, mainApp, &MainApplication::editShapeLetterSpacing);
  connect(font_panel_, &FontPanel::editShapeBold, mainApp, &MainApplication::editShapeBold);
  connect(font_panel_, &FontPanel::editShapeItalic, mainApp, &MainApplication::editShapeItalic);
  connect(font_panel_, &FontPanel::editShapeUnderline, mainApp, &MainApplication::editShapeUnderline);
  connect(font_panel_, &FontPanel::editShapeLineHeight, mainApp, &MainApplication::editShapeLineHeight);
  connect(mainApp, &MainApplication::updateFontView, font_panel_, &FontPanel::updateFontView);

  //about image
  connect(image_panel_, &ImagePanel::editImageGradient, mainApp, &MainApplication::updateImageGradient);
  connect(image_panel_, &ImagePanel::editImageThreshold, mainApp, &MainApplication::updateImageThreshold);
  connect(image_panel_, &ImagePanel::actionCropImage, canvas_, &Canvas::cropImage);
  connect(image_panel_, &ImagePanel::actionInvertImage, canvas_, &Canvas::invertImage);
  connect(image_panel_, &ImagePanel::actionSharpenImage, canvas_, &Canvas::sharpenImage);
  connect(image_panel_, &ImagePanel::actionGenImageTrace, canvas_, &Canvas::genImageTrace);
  connect(mainApp, &MainApplication::editImageGradient, [=](bool state) {
    image_panel_->setImageGradient(state);
    QList<ShapePtr> shape_list = canvas_->document().selections();
    if(shape_list.size() == 1 && shape_list.at(0)->type() == ::Shape::Type::Bitmap) {
      BitmapShape* selected_img = dynamic_cast<BitmapShape *>(shape_list.at(0).get());
      selected_img->setGradient(state);
      canvas_->shapeUpdated();
    }
  });

  //about preset
  connect(mainApp, &MainApplication::editPresetIndex, [=](int preset_index, int param_index) {
    doc_panel_->setPresetIndex(preset_index);
    layer_panel_->setPresetIndex(preset_index, param_index);
    canvas_->document().activeLayer()->setParameterIndex(param_index);
  });
  connect(mainApp, &MainApplication::editFramingPower, [=](double power) {
    jogging_panel_->setFramingPower(power);
  });
  connect(mainApp, &MainApplication::editPulsePower, [=](double power) {
    jogging_panel_->setPulsePower(power);
  });
  connect(doc_panel_, &DocPanel::updatePresetIndex, [=](int index) {
    mainApp->updatePresetIndex(index);
  });
  connect(layer_panel_, &LayerPanel::editParamIndex, [=](int index) {
    mainApp->updateParamIndex(index);
  });
  connect(layer_panel_, &LayerPanel::wakeupPresetManager, [=]() {
    PresetManager *preset_manager = new PresetManager(this, mainApp->getPresetIndex());
    connect(preset_manager, &PresetManager::updateCurrentPresetIndex, [=](int index) {
      mainApp->updatePresetIndex(index);
    });
    connect(preset_manager, &PresetManager::updateCurrentIndex, [=](int preset_index, int param_index) {
      mainApp->updatePresetIndex(preset_index, param_index);
    });
    doc_panel_->setPresetSelectLock(false);
    layer_panel_->setLayerParamLock(false);
    preset_manager->show();
    preset_manager->activateWindow();
    preset_manager->raise();
    if(preset_manager->exec() != 1) {
      Q_EMIT mainApp->editPresetIndex(mainApp->getPresetIndex(), mainApp->getParamIndex());
    }
    doc_panel_->setPresetSelectLock(true);
    layer_panel_->setLayerParamLock(true);
  });
  connect(layer_panel_, &LayerPanel::editLayerParam, [=](double strength, double speed, int repeat) {
    canvas_->document().activeLayer()->setStrength(strength);
    canvas_->document().activeLayer()->setSpeed(speed);
    canvas_->document().activeLayer()->setRepeat(repeat);
  });
  connect(layer_panel_, &LayerPanel::editLayerBacklash, [=](double backlash) {
    canvas_->document().activeLayer()->setXBacklash(backlash);
  });
  connect(jogging_panel_, &JoggingPanel::updateFramingPower, [=](double framing_power) {
    mainApp->updateFramingPower(framing_power);
  });
  connect(jogging_panel_, &JoggingPanel::updatePulsePower, [=](double pulse_power) {
    mainApp->updatePulsePower(pulse_power);
  });
  //about machine
  connect(doc_panel_, &DocPanel::updateMachineIndex, [=](int machine_index) {
    mainApp->updateMachineIndex(machine_index);
  });
  connect(doc_panel_, &DocPanel::updateTravelSpeed, [=](double travel_speed) {
    mainApp->updateMachineTravelSpeed(travel_speed);
  });
  connect(machine_manager_, &MachineManager::updateCurrentMachineIndex, [=](int index) {
    mainApp->updateMachineIndex(index);
  });
  connect(mainApp, &MainApplication::editMachineIndex, [=](int machine_index) {
    doc_panel_->setMachineIndex(machine_index);
    active_machine.applyMachineParam(mainApp->getMachineParam());
  });
  connect(mainApp, &MainApplication::editMachineTravelSpeed, [=](double travel_speed) {
    doc_panel_->setTravelSpeed(travel_speed);
  });
  connect(mainApp, &MainApplication::editWorkingRange, this, &MainWindow::updateCanvasSize);
  connect(mainApp, &MainApplication::editMachineRotaryAxis, [=](char axis) {
    machine_manager_->setRotaryAxis(axis);
    rotary_setup_->setRotaryAxis(axis);
  });
  connect(mainApp, &MainApplication::editMachineHighSpeedMode, [=](bool is_high_speed) {
    preferences_window_->setSpeedMode(is_high_speed);
    showHighSpeedWarning();
  });
  //about rotary
  connect(doc_panel_, &DocPanel::updateRotarySpeed, [=](double travel_speed) {
    mainApp->updateRotarySpeed(travel_speed);
  });
  connect(doc_panel_, &DocPanel::rotaryModeChange, [=](bool rotary_mode) {
    mainApp->updateRotaryMode(rotary_mode);
  });
  connect(rotary_setup_, &RotarySetup::updateRotaryIndex, [=](int rotary_index) {
    mainApp->updateRotaryIndex(rotary_index);
  });
  connect(rotary_setup_, &RotarySetup::rotaryModeChanged, [=](bool is_rotary_mode) {
    mainApp->updateRotaryMode(is_rotary_mode);
  });
  connect(rotary_setup_, &RotarySetup::mirrorModeChanged, [=](bool is_mirror_mode) {
    mainApp->updateMirrorMode(is_mirror_mode);
  });
  connect(rotary_setup_, &RotarySetup::rotaryAxisChanged, [=](char axis) {
    mainApp->updateMachineRotaryAxis(axis);
  });
  connect(rotary_setup_, &RotarySetup::actionTestRotary, [=](double test_distance) {
    QRectF test_rect(0, 0, 20, test_distance);
    testRotary(test_rect, mainApp->getRotaryAxis(), mainApp->getTravelSpeed() * 60, mainApp->getFramingPower());// mm/s to mm/min
  });
  connect(rotary_setup_, &RotarySetup::updateCircumference, [=](double circumference) {
    mainApp->updateCircumference(circumference);
  });
  connect(rotary_setup_, &RotarySetup::updateRotarySpeed, [=](double travel_speed) {
    mainApp->updateRotarySpeed(travel_speed);
  });
  connect(mainApp, &MainApplication::editRotaryMode, [=](bool rotary_mode) {
    doc_panel_->setRotaryMode(rotary_mode);
    rotary_setup_->setRotaryMode(rotary_mode);
    if(rotary_mode) {
      mode_block_->setText(tr("Rotary Mode"));
      mainApp->updateReferenceStartWithHome(false);
      laser_panel_->setStartHomeEnable(!rotary_mode);
    } else {
      mode_block_->setText(tr("XY Mode"));
      mainApp->updateReferenceStartWithHome(laser_panel_->getStartHome());
      laser_panel_->setStartHomeEnable(!rotary_mode);
    }
  });
  connect(mainApp, &MainApplication::editRotaryIndex, [=](int rotary_index) {
  });
  connect(mainApp, &MainApplication::editRotaryTravelSpeed, [=](double speed) {
    doc_panel_->setRotarySpeed(speed);
  });
  connect(mainApp, &MainApplication::editCircumference, [=](double circumference) {});
  //fail to update image panel qslider
  connect(mainApp, &MainApplication::editImageThreshold, [=](int value) {
    image_panel_->setImageThreshold(value);
    QList<ShapePtr> shape_list = canvas_->document().selections();
    if(shape_list.size() == 1 && shape_list.at(0)->type() == ::Shape::Type::Bitmap) {
      BitmapShape* selected_img = dynamic_cast<BitmapShape *>(shape_list.at(0).get());
      selected_img->setThrshBrightness(value);
      canvas_->shapeUpdated();
    }
  });
  connect(mainApp, &MainApplication::changeImageEnable, [=](bool state) {
    image_panel_->changeImageEnable(state);
    ui->actionTrace->setEnabled(state);
    ui->actionInvert->setEnabled(state);
    ui->actionReplace_with->setEnabled(state);
    ui->actionCrop->setEnabled(state);
    ui->actionSharpen->setEnabled(state);
  });
  //about canvas
  connect(mainApp, &MainApplication::editCanvasQuality, [=](CanvasQuality canvas_quality) {
    preferences_window_->setCanvasQuality(canvas_quality);
    canvas_->setCanvasQuality(canvas_quality);
  });
  connect(mainApp, &MainApplication::editPathSort, [=](PathSort path_sort) {
    preferences_window_->setPathSort(path_sort);
  });

  connect(gcode_panel_, &GCodePanel::exportGcode, this, &MainWindow::exportGCodeFile);
  connect(gcode_panel_, &GCodePanel::importGcode, this, &MainWindow::importGCodeFile);
  connect(gcode_panel_, &GCodePanel::generateGcode, this, &MainWindow::generateGcode);
  connect(gcode_panel_, &GCodePanel::startBtnClicked, this, &MainWindow::onStartNewJob);
  connect(gcode_panel_, &GCodePanel::pauseBtnClicked, this, &MainWindow::onPauseJob);
  connect(gcode_panel_, &GCodePanel::resumeBtnClicked, this, &MainWindow::onResumeJob);
  connect(gcode_panel_, &GCodePanel::stopBtnClicked, this, &MainWindow::onStopJob);
  connect(gcode_panel_, &GCodePanel::jobStatusReport, this, &MainWindow::syncJobState);

  connect(laser_panel_, &LaserPanel::actionPreview, this, &MainWindow::genPreviewWindow);
  connect(laser_panel_, &LaserPanel::actionFrame, this, &MainWindow::actionFrame);
  connect(laser_panel_, &LaserPanel::actionStart, this, &MainWindow::actionStart);
  connect(laser_panel_, &LaserPanel::actionHome, this, &MainWindow::home);
  connect(laser_panel_, &LaserPanel::actionMoveToOrigin, this, &MainWindow::moveToCustomOrigin);
  connect(laser_panel_, &LaserPanel::selectJobOrigin, mainApp, &MainApplication::updateReferenceJobOrigin);
  connect(laser_panel_, &LaserPanel::switchStartFrom, mainApp, &MainApplication::updateReferenceStartFrom);
  connect(laser_panel_, &LaserPanel::startWithHome, mainApp, &MainApplication::updateReferenceStartWithHome);
  connect(mainApp, &MainApplication::editReferenceJobOrigin, [=](int job_origin) {
    laser_panel_->setJobOrigin(job_origin);
  });
  connect(mainApp, &MainApplication::editReferenceStartFrom, [=](int start_from) {
    switch(start_from) {
      case StartFrom::AbsoluteCoords:
        canvas_->setJobOrigin(false);
        break;
      case StartFrom::UserOrigin:
      case StartFrom::CurrentPosition:
        canvas_->setJobOrigin(true);
        break;
      default:
        break;
    }
    laser_panel_->setStartFrom(start_from);
  });
  //keep the start with home at laser panel
  connect(mainApp, &MainApplication::editReferenceStartWithHome, [=](bool find_home) {
  });

  connect(jogging_panel_, &JoggingPanel::actionLaser, this, &MainWindow::laser);
  connect(jogging_panel_, &JoggingPanel::actionLaserPulse, this, &MainWindow::laserPulse);
  connect(jogging_panel_, &JoggingPanel::actionHome, this, &MainWindow::home);
  connect(jogging_panel_, &JoggingPanel::actionMoveRelatively, [=](qreal x, qreal y) {
    moveRelatively(x, y, mainApp->getTravelSpeed() * 60);// mm/s to mm/min
  });
  connect(jogging_panel_, &JoggingPanel::actionMoveAbsolutely, [=](std::tuple<qreal, qreal, qreal>pos) {
    moveAbsolutely(pos, mainApp->getTravelSpeed() * 60);// mm/s to mm/min
  });
  connect(jogging_panel_, &JoggingPanel::actionMoveToEdge, [=](int edge_id) {
    moveToEdge(edge_id, mainApp->getTravelSpeed() * 60);// mm/s to mm/min
  });
  connect(jogging_panel_, &JoggingPanel::actionMoveToCorner, [=](int corner_id) {
    moveToCorner(corner_id, mainApp->getTravelSpeed() * 60);// mm/s to mm/min
  });
  connect(jogging_panel_, &JoggingPanel::actionSetOrigin, this, &MainWindow::setCustomOrigin);
  connect(jogging_panel_, &JoggingPanel::stopBtnClicked, this, &MainWindow::onStopJob);
  connect(jogging_panel_, &JoggingPanel::showCurrentPosition, [=](bool show) {
    canvas_->setCurrentPosition(show);
  });
  connect(jogging_panel_, &JoggingPanel::showUserOrigin, [=](bool show) {
    canvas_->setUserOrigin(show);
  });

  //panel
  connect(transform_panel_, &TransformPanel::panelShow, [=](bool is_show) {
    ui->actionObjectPanel->setChecked(is_show);
  });
  connect(font_panel_, &FontPanel::panelShow, [=](bool is_show) {
    ui->actionFontPanel->setChecked(is_show);
  });
  connect(image_panel_, &ImagePanel::panelShow, [=](bool is_show) {
    ui->actionImagePanel->setChecked(is_show);
  });
  connect(doc_panel_, &DocPanel::panelShow, [=](bool is_show) {
    ui->actionDocumentPanel->setChecked(is_show);
  });
  connect(laser_panel_, &LaserPanel::panelShow, [=](bool is_show) {
    ui->actionLaser->setChecked(is_show);
  });
  connect(layer_panel_, &LayerPanel::panelShow, [=](bool is_show) {
    ui->actionLayerPanel->setChecked(is_show);
  });
  connect(gcode_panel_, &GCodePanel::panelShow, [=](bool is_show) {
    ui->actionGCodeViewerPanel->setChecked(is_show);
  });
  connect(jogging_panel_, &JoggingPanel::panelShow, [=](bool is_show) {
    ui->actionJoggingPanel->setChecked(is_show);
  });
  //toolbar
  connect(ui->toolBarAlign, &QToolBar::visibilityChanged, [=](bool is_visible) {
    ui->actionAlign->setChecked(is_visible);
  });
  connect(ui->toolBarBool, &QToolBar::visibilityChanged, [=](bool is_visible) {
    ui->actionBoolean->setChecked(is_visible);
  });
  connect(ui->toolBarConnection, &QToolBar::visibilityChanged, [=](bool is_visible) {
    ui->actionConnection->setChecked(is_visible);
  });
  connect(ui->toolBarFlip, &QToolBar::visibilityChanged, [=](bool is_visible) {
    ui->actionFlip->setChecked(is_visible);
  });
  connect(ui->toolBarFont, &QToolBar::visibilityChanged, [=](bool is_visible) {
    ui->actionFont->setChecked(is_visible);
  });
  connect(ui->toolBar, &QToolBar::visibilityChanged, [=](bool is_visible) {
    ui->actionFunctional->setChecked(is_visible);
  });
  connect(ui->toolBarGroup, &QToolBar::visibilityChanged, [=](bool is_visible) {
    ui->actionGroup_2->setChecked(is_visible);
  });
  connect(ui->toolBarImage, &QToolBar::visibilityChanged, [=](bool is_visible) {
    ui->actionImage->setChecked(is_visible);
  });
  connect(ui->toolBarFile, &QToolBar::visibilityChanged, [=](bool is_visible) {
    ui->actionProject->setChecked(is_visible);
  });
  connect(ui->toolBarTask, &QToolBar::visibilityChanged, [=](bool is_visible) {
    ui->actionTask->setChecked(is_visible);
  });
  connect(ui->toolBarTransform, &QToolBar::visibilityChanged, [=](bool is_visible) {
    ui->actionTransform->setChecked(is_visible);
  });
  connect(ui->toolBarVector, &QToolBar::visibilityChanged, [=](bool is_visible) {
    ui->actionVector->setChecked(is_visible);
  });

  // TODO: Refactor it when supporting multi-port and multi-machine connection
  //       NOTE: The active_machine might be null at the beginning
  #ifdef CUSTOM_SERIAL_PORT_LIB
  #endif
  connect(&active_machine, &Machine::connected, [=]() {
    // Port connected but hasn't responded any meaningful response
    if (!console_dialog_.isNull()) {
      connect(&active_machine, &Machine::logSent, console_dialog_.data(), &ConsoleDialog::appendLogSent, Qt::UniqueConnection);
      connect(&active_machine, &Machine::logRcvd, console_dialog_.data(), &ConsoleDialog::appendLogRcvd, Qt::UniqueConnection);
    }
    ui->actionConnect->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-connecting.png" : ":/resources/images/icon-connecting.png"));
  });
  connect(&active_machine, &Machine::activated, [=]() {
    Q_EMIT MainWindow::activeMachineConnected();
    ui->actionConnect->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-link.png" : ":/resources/images/icon-link.png"));
    ui->actionFrame->setEnabled(true);
    jogging_panel_->setControlEnable(true);
    laser_panel_->setControlEnable(true);
  });

#if defined(HAVE_SOFTWARE_UPDATE) && defined(Q_OS_WIN)
  connect(mainApp, &MainApplication::softwareUpdateRequested, this, &MainWindow::softwareUpdateRequested,
      Qt::BlockingQueuedConnection);
  connect(mainApp, &MainApplication::softwareUpdateClose, this, &QMainWindow::close,
      Qt::BlockingQueuedConnection);
#endif
}

void MainWindow::closeEvent(QCloseEvent *event) {
  if (!handleUnsavedChange()) {
    event->ignore();
  }
  else{
    // QSettings settings;
    QSettings settings("flux", "swiftray");
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/windowState", saveState());
    event->accept();
  }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
  scale_block_->setGeometry(ui->quickWidget->geometry().left() + 60, this->size().height() - 150, 50, 30);
  minusBtn_->setGeometry(ui->quickWidget->geometry().left() + 30, this->size().height() - 150, 40, 30);
  plusBtn_->setGeometry(ui->quickWidget->geometry().left() + 100, this->size().height() - 150, 40, 30);
}

Canvas *MainWindow::canvas() const {
  return canvas_;
}

bool isDarkMode() {
#ifdef Q_OS_MACOS
  return isOSXDarkMode();
#endif
  return false;
}

void MainWindow::setCanvasContextMenu() {
  popMenu_ = new QMenu(ui->quickWidget);
  // Add QActions for context menu
  cutAction_ = popMenu_->addAction(tr("Cut"));
  copyAction_ =  popMenu_->addAction(tr("Copy"));
  pasteAction_ =  popMenu_->addAction(tr("Paste"));
  pasteInPlaceAction_ =  popMenu_->addAction(tr("Paste in Place"));
  duplicateAction_ = popMenu_->addAction(tr("Duplicate"));
  popMenu_->addSeparator();
  deleteAction_ = popMenu_->addAction(tr("Delete"));
  popMenu_->addSeparator();
  groupAction_ = popMenu_->addAction(tr("group"));
  ungroupAction_ = popMenu_->addAction(tr("ungroup"));

  ui->quickWidget->addAction(cutAction_);
  ui->quickWidget->addAction(copyAction_);
  ui->quickWidget->addAction(pasteAction_);
  ui->quickWidget->addAction(pasteInPlaceAction_);
  ui->quickWidget->addAction(duplicateAction_);
  ui->quickWidget->addAction(deleteAction_);
  ui->quickWidget->addAction(groupAction_);
  ui->quickWidget->addAction(ungroupAction_);

  ui->quickWidget->connect(cutAction_, &QAction::triggered, canvas_, &Canvas::editCut);
  ui->quickWidget->connect(copyAction_, &QAction::triggered, canvas_, &Canvas::editCopy);
  ui->quickWidget->connect(pasteAction_, &QAction::triggered, canvas_, &Canvas::editPasteInRightButton);
  ui->quickWidget->connect(pasteInPlaceAction_, &QAction::triggered, canvas_, &Canvas::editPasteInPlace);
  ui->quickWidget->connect(duplicateAction_, &QAction::triggered, canvas_, &Canvas::editDuplicate);
  ui->quickWidget->connect(deleteAction_, &QAction::triggered, canvas_, &Canvas::editCut);
  ui->quickWidget->connect(groupAction_, &QAction::triggered, canvas_, &Canvas::editGroup);
  ui->quickWidget->connect(ungroupAction_, &QAction::triggered, canvas_, &Canvas::editUngroup);

  ui->quickWidget->setContextMenuPolicy(Qt::CustomContextMenu);
}

void MainWindow::setConnectionToolBar() {
  baudComboBox_ = new QComboBox;
  portComboBox_ = new QComboBox;
  portComboBox_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  ui->toolBarConnection->insertWidget(ui->actionConnect, portComboBox_);
  ui->toolBarConnection->insertWidget(ui->actionConnect, baudComboBox_);
  ui->actionConnect->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-unlink.png" : ":/resources/images/icon-unlink.png"));
  QTimer *timer = new QTimer(this);
  QList<QSerialPortInfo> portList;
  baudComboBox_->addItem("9600");
  baudComboBox_->addItem("19200");
  baudComboBox_->addItem("38400");
  baudComboBox_->addItem("57600");
  baudComboBox_->addItem("115200");
  baudComboBox_->addItem("230400");
  baudComboBox_->addItem("460800");
  baudComboBox_->addItem("921600");
  baudComboBox_->setCurrentIndex(4); // default baudrate 115200
  
  connect(timer, &QTimer::timeout, [=]() {
    const auto infos = QSerialPortInfo::availablePorts();
    int current_index = portComboBox_->currentIndex() > -1 ? portComboBox_->currentIndex() : 0;
    portComboBox_->clear();
    for (const QSerialPortInfo &info : infos) {
      if(info.portName().toStdString().compare(0,3,"cu.") == 0 ||
      info.portName().toStdString().compare(0,13,"tty.Bluetooth") == 0) {}
      else {
        portComboBox_->addItem(info.portName());
      }
    }
    portComboBox_->setCurrentIndex(current_index > portComboBox_->count() - 1 ? portComboBox_->count() - 1 : current_index);

    // Check whether port is unplugged
    #ifdef CUSTOM_SERIAL_PORT_LIB
    if (serial_port.isOpen()) {
      auto matchIt = std::find_if(
              infos.begin(), infos.end(),
              [](const QSerialPortInfo& info) { return info.portName() == serial_port.portName(); }
      );
      if (matchIt == infos.end()) { // Unplugged -> close opened port
        qInfo() << "Serial port unplugged";
        serial_port.close();
      }
    }
    #endif

  });
  connect(ui->actionConnect, &QAction::triggered, [=]() {
    if (serial_port.isOpen()) {
      if (active_machine.getMotionController()) {
        active_machine.getMotionController()->detachPort();
        qInfo() << "[Port] Detached";
      } else {
        qInfo() << "[SerialPort] Disconnect";
        serial_port.close(); // disconnect
      }
      return;
    }
    QString port_name = portComboBox_->currentText();
    QString baudrate = baudComboBox_->currentText();
    qInfo() << "[SerialPort] Connecting" << port_name << baudrate;
    #ifdef CUSTOM_SERIAL_PORT_LIB
    serial_port.open(port_name, baudrate.toInt());
    #else
    serial_port.setPortName(port_name);
    serial_port.setBaudRate(baudrate.toInt());
    serial_port.open(QIODevice::ReadWrite);
    #endif
    if (!serial_port.isOpen()) {
      QMessageBox msgbox;
      msgbox.setText(tr("Error"));
      msgbox.setInformativeText(tr("Unable to connect to the port.  Make sure no existing program is using it."));
      msgbox.exec();
      return;
    }
    #ifdef CUSTOM_SERIAL_PORT_LIB
    #else
    disconnect(&serial_port, nullptr, nullptr, nullptr);
    serial_port.clearError();
    #endif
    active_machine.motionPortConnected(&serial_port);
    active_machine.applyMachineParam(mainApp->getMachineParam());
    connect(&active_machine, &Machine::positionCached, this, &MainWindow::machinePositionCached, Qt::UniqueConnection);
    connect(&active_machine, &Machine::disconnected, this, &MainWindow::machineDisconnected, Qt::UniqueConnection);
  });
  timer->start(1500);
}

void MainWindow::setToolbarFont() {
  auto fontComboBox = new QFontComboBox(ui->toolBarFont);
  ui->toolBarFont->addWidget(fontComboBox);
  //auto labelStyle = new QLabel(tr("Style"), ui->toolBarFont);
  //ui->toolBarFont->addWidget(labelStyle);
  auto styleHBoxLayout = new QHBoxLayout(ui->toolBarFont);
  auto boldToolButton = new QToolButton(ui->toolBarFont);
  auto italicToolButton = new QToolButton(ui->toolBarFont);
  auto underlineToolButton = new QToolButton(ui->toolBarFont);
  auto spin_event = QOverload<double>::of(&QDoubleSpinBox::valueChanged);
  auto spin_int_event = QOverload<int>::of(&QSpinBox::valueChanged);
  ui->toolBarFont->setStyleSheet("\
    QToolButton {   \
        border: none; \
        margin: 0; \
        padding: 0; \
    } \
    QToolButton:checked{ \
        border: none \
    } \
  ");
  boldToolButton->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-Bold.png" : ":/resources/images/icon-Bold.png"));
  italicToolButton->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-I.png" : ":/resources/images/icon-I.png"));
  underlineToolButton->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-U.png" : ":/resources/images/icon-U.png"));
  boldToolButton->setCheckable(true);
  italicToolButton->setCheckable(true);
  underlineToolButton->setCheckable(true);
  ui->toolBarFont->addWidget(boldToolButton);
  ui->toolBarFont->addWidget(italicToolButton);
  ui->toolBarFont->addWidget(underlineToolButton);
  auto labelSize = new QLabel(tr("Size"), ui->toolBarFont);
  auto labelLineHeight = new QLabel(tr("Line Height"), ui->toolBarFont);
  auto labelLetterSpacing = new QLabel(tr("Letter Spacing"), ui->toolBarFont);
  auto spinBoxSize = new QSpinBox(ui->toolBarFont);
  auto doubleSpinBoxLineHeight = new QDoubleSpinBox2(ui->toolBarFont);
  auto doubleSpinBoxLetterSpacing = new QDoubleSpinBox2(ui->toolBarFont);

  doubleSpinBoxLetterSpacing->setDecimals(1);
  doubleSpinBoxLetterSpacing->setMaximum(1000);
  doubleSpinBoxLetterSpacing->setMinimum(-0.1);
  doubleSpinBoxLetterSpacing->setSingleStep(0.1);
  doubleSpinBoxLineHeight->setDecimals(1);
  doubleSpinBoxLineHeight->setMaximum(100);
  doubleSpinBoxLineHeight->setSingleStep(0.1);
  spinBoxSize->setMaximum(1000);

  //Initial panel setting by MainApplication
  fontComboBox->setCurrentFont(mainApp->getFont());
  spinBoxSize->setValue(mainApp->getFont().pointSize());
  doubleSpinBoxLetterSpacing->setValue(mainApp->getFont().letterSpacing());
  boldToolButton->setChecked(mainApp->getFont().bold());
  italicToolButton->setChecked(mainApp->getFont().italic());
  underlineToolButton->setChecked(mainApp->getFont().underline());
  doubleSpinBoxLineHeight->setValue(mainApp->getFontLineHeight());

  ui->toolBarFont->addWidget(labelSize);
  ui->toolBarFont->addWidget(spinBoxSize);
  ui->toolBarFont->addWidget(labelLineHeight);
  ui->toolBarFont->addWidget(doubleSpinBoxLineHeight);
  ui->toolBarFont->addWidget(labelLetterSpacing);
  ui->toolBarFont->addWidget(doubleSpinBoxLetterSpacing);

  connect(mainApp, &MainApplication::editShapeFontFamily, [=](QString font_family) {
    canvas_->setFontFamily(font_family);
    font_panel_->setFontFamily(font_family);
    fontComboBox->setCurrentFont(QFont(font_family));
  });

  connect(mainApp, &MainApplication::editShapeFontPointSize, [=](int point_size) {
    canvas_->setPointSize(point_size);
    font_panel_->setPointSize(point_size);
    spinBoxSize->setValue(point_size);
  });

  connect(mainApp, &MainApplication::editShapeLetterSpacing, [=](qreal letter_spacing) {
    canvas_->setLetterSpacing(letter_spacing);
    font_panel_->setLetterSpacing(letter_spacing);
    doubleSpinBoxLetterSpacing->setValue(letter_spacing);
  });

  connect(mainApp, &MainApplication::editShapeLineHeight, [=](double line_height) {
    canvas_->setLineHeight(line_height);
    font_panel_->setLineHeight(line_height);
    doubleSpinBoxLineHeight->setValue(line_height);
  });

  connect(mainApp, &MainApplication::editShapeBold, [=](bool bold) {
    canvas_->setBold(bold);
    font_panel_->setBold(bold);
    boldToolButton->setChecked(bold);
  });

  connect(mainApp, &MainApplication::editShapeItalic, [=](bool italic) {
    canvas_->setItalic(italic);
    font_panel_->setItalic(italic);
    italicToolButton->setChecked(italic);
  });

  connect(mainApp, &MainApplication::editShapeUnderline, [=](bool underline) {
    canvas_->setUnderline(underline);
    font_panel_->setUnderline(underline);
    underlineToolButton->setChecked(underline);
  });

  connect(fontComboBox, &QFontComboBox::currentFontChanged, 
          mainApp, &MainApplication::updateShapeFontFamily);

  connect(spinBoxSize, spin_int_event, 
          mainApp, &MainApplication::updateShapeFontPointSize);

  connect(doubleSpinBoxLetterSpacing, spin_event, 
          mainApp, &MainApplication::updateShapeLetterSpacing);
  
  connect(doubleSpinBoxLineHeight, spin_event, 
          mainApp, &MainApplication::updateShapeLineHeight);
  
  connect(boldToolButton, &QToolButton::toggled, 
          mainApp, &MainApplication::updateShapeBold);
  
  connect(italicToolButton, &QToolButton::toggled, 
          mainApp, &MainApplication::updateShapeItalic);
  
  connect(underlineToolButton, &QToolButton::toggled, 
          mainApp, &MainApplication::updateShapeUnderline);

  //this part must follow font panel
  connect(mainApp, &MainApplication::updateFontView, [=](
          QSet<QString> font_familys, 
          QSet<int> point_sizes, 
          QSet<qreal> letter_spacings, 
          QSet<bool> bolds, 
          QSet<bool> italics, 
          QSet<bool> underlines, 
          QSet<double> line_heights) {
            if(font_familys.size() == 1) {
              fontComboBox->setCurrentFont(QFont(*font_familys.begin()));
            } else if(!font_familys.empty()) {
              fontComboBox->blockSignals(true);
              fontComboBox->setCurrentText("");
              fontComboBox->blockSignals(false);
            }
            if(point_sizes.size() == 1) {
              spinBoxSize->setValue(*point_sizes.begin());
            } else if(!point_sizes.empty()) {
              spinBoxSize->blockSignals(true);
              spinBoxSize->setSpecialValueText(tr(" "));
              spinBoxSize->setValue(0);
              spinBoxSize->blockSignals(false);
            }
            if(letter_spacings.size() == 1) {
              doubleSpinBoxLetterSpacing->setValue(*letter_spacings.begin());
            } else if(!letter_spacings.empty()) {
              doubleSpinBoxLetterSpacing->blockSignals(true);
              doubleSpinBoxLetterSpacing->setSpecialValueText(tr(" "));
              doubleSpinBoxLetterSpacing->setValue(-0.1);
              doubleSpinBoxLetterSpacing->blockSignals(false);
            }
            if(bolds.size() == 1) {
              boldToolButton->setChecked(*bolds.begin());
            } else if(!bolds.empty()) {
              boldToolButton->blockSignals(true);
              boldToolButton->setChecked(false);
              boldToolButton->blockSignals(false);
            }
            if(italics.size() == 1) {
              italicToolButton->setChecked(*italics.begin());
            } else if(!italics.empty()) {
              italicToolButton->blockSignals(true);
              italicToolButton->setChecked(false);
              italicToolButton->blockSignals(false);
            }
            if(underlines.size() == 1) {
              underlineToolButton->setChecked(*underlines.begin());
            } else if(!underlines.empty()) {
              underlineToolButton->blockSignals(true);
              underlineToolButton->setChecked(false);
              underlineToolButton->blockSignals(false);
            }
            if(line_heights.size() == 1) {
              doubleSpinBoxLineHeight->setValue(*line_heights.begin());
            } else if(!line_heights.empty()) {
              doubleSpinBoxLineHeight->blockSignals(true);
              doubleSpinBoxLineHeight->setSpecialValueText(tr(" "));
              doubleSpinBoxLineHeight->setValue(0);
              doubleSpinBoxLineHeight->blockSignals(false);
            }
          });
  connect(mainApp, &MainApplication::changeFontEnable, [=](bool enable) {
    font_panel_->changeFontEnable(enable);
    fontComboBox->setEnabled(enable);
    spinBoxSize->setEnabled(enable);
    doubleSpinBoxLetterSpacing->setEnabled(enable);
    boldToolButton->setEnabled(enable);
    italicToolButton->setEnabled(enable);
    underlineToolButton->setEnabled(enable);
    doubleSpinBoxLineHeight->setEnabled(enable);
  });
  font_panel_->changeFontEnable(false);
  fontComboBox->setEnabled(false);
  spinBoxSize->setEnabled(false);
  doubleSpinBoxLetterSpacing->setEnabled(false);
  boldToolButton->setEnabled(false);
  italicToolButton->setEnabled(false);
  underlineToolButton->setEnabled(false);
  doubleSpinBoxLineHeight->setEnabled(false);
}

void MainWindow::setToolbarTransform() {
  auto labelX = new QLabel;
  auto doubleSpinBoxX = new QDoubleSpinBox2(ui->toolBarTransform);
  auto labelY = new QLabel;
  auto doubleSpinBoxY = new QDoubleSpinBox2(ui->toolBarTransform);
  auto labelRotation = new QLabel;
  auto doubleSpinBoxRotation = new QDoubleSpinBox2(ui->toolBarTransform);
  auto labelWidth = new QLabel;
  auto doubleSpinBoxWidth = new QDoubleSpinBox2(ui->toolBarTransform);
  auto labelHeight = new QLabel;
  auto doubleSpinBoxHeight = new QDoubleSpinBox2(ui->toolBarTransform);
  auto buttonLock = new QToolButton(ui->toolBarTransform);
  ui->toolBarTransform->setStyleSheet("\
      QToolButton {   \
          border: none \
      } \
      QToolButton:checked{ \
          border: none \
      } \
  ");
  ui->toolBarTransform->setIconSize(QSize(16, 16));
  buttonLock->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-unlock.png" : ":/resources/images/icon-unlock.png"));
  buttonLock->setCheckable(true);
  labelX->setText("X");
  labelY->setText("Y");
  labelRotation->setText(tr("Rotation"));
  labelWidth->setText(tr("Width"));
  labelHeight->setText(tr("Height"));
  doubleSpinBoxX->setMaximum(9999);
  doubleSpinBoxY->setMaximum(9999);
  doubleSpinBoxX->setMinimum(-9999);
  doubleSpinBoxY->setMinimum(-9999);
  doubleSpinBoxRotation->setMaximum(360);
  doubleSpinBoxRotation->setMinimum(-360);
  doubleSpinBoxWidth->setMaximum(9999);
  doubleSpinBoxHeight->setMaximum(9999);
  doubleSpinBoxX->setSuffix(" mm");
  doubleSpinBoxY->setSuffix(" mm");
  doubleSpinBoxWidth->setSuffix(" mm");
  doubleSpinBoxHeight->setSuffix(" mm");
  ui->toolBarTransform->addWidget(labelX);
  ui->toolBarTransform->addWidget(doubleSpinBoxX);
  ui->toolBarTransform->addWidget(labelY);
  ui->toolBarTransform->addWidget(doubleSpinBoxY);
  ui->toolBarTransform->addWidget(labelWidth);
  ui->toolBarTransform->addWidget(doubleSpinBoxWidth);
  ui->toolBarTransform->addWidget(buttonLock);
  ui->toolBarTransform->addWidget(labelHeight);
  ui->toolBarTransform->addWidget(doubleSpinBoxHeight);
  ui->toolBarTransform->addWidget(labelRotation);
  ui->toolBarTransform->addWidget(doubleSpinBoxRotation);
  buttonLock->setChecked(mainApp->isShapeScaleLocked());

  auto spin_event = QOverload<double>::of(&QDoubleSpinBox::valueChanged);

  connect(mainApp, &MainApplication::editShapeTransform, [=](qreal x, qreal y, qreal r, qreal w, qreal h) {
    canvas_->transformControl().updateTransform(x * 10, y * 10, r, w * 10, h * 10);
    transform_panel_->setTransformX(x);
    transform_panel_->setTransformY(y);
    transform_panel_->setTransformR(r);
    transform_panel_->setTransformW(w);
    transform_panel_->setTransformH(h);
    doubleSpinBoxX->blockSignals(true);
    doubleSpinBoxY->blockSignals(true);
    doubleSpinBoxRotation->blockSignals(true);
    doubleSpinBoxWidth->blockSignals(true);
    doubleSpinBoxHeight->blockSignals(true);
    doubleSpinBoxX->setValue(x);
    doubleSpinBoxY->setValue(y);
    doubleSpinBoxRotation->setValue(r);
    doubleSpinBoxWidth->setValue(w);
    doubleSpinBoxHeight->setValue(h);
    doubleSpinBoxX->blockSignals(false);
    doubleSpinBoxY->blockSignals(false);
    doubleSpinBoxRotation->blockSignals(false);
    doubleSpinBoxWidth->blockSignals(false);
    doubleSpinBoxHeight->blockSignals(false);
  });

  connect(mainApp, &MainApplication::editShapeScaleLock, [=](bool scale_locked) {
    canvas_->transformControl().setScaleLock(scale_locked);
    transform_panel_->setScaleLock(scale_locked);
    buttonLock->blockSignals(true);
    buttonLock->setChecked(scale_locked);
    buttonLock->blockSignals(false);

    if (scale_locked) {
      buttonLock->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-lock.png" : ":/resources/images/icon-lock.png"));
    } else {
      buttonLock->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-unlock.png" : ":/resources/images/icon-unlock.png"));
    }
  });

  connect(doubleSpinBoxX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
          mainApp, &MainApplication::updateShapeTransformX);

  connect(doubleSpinBoxY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
          mainApp, &MainApplication::updateShapeTransformY);

  connect(doubleSpinBoxRotation, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
          mainApp, &MainApplication::updateShapeTransformR);

  connect(doubleSpinBoxWidth, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
          mainApp, &MainApplication::updateShapeTransformW);

  connect(doubleSpinBoxHeight, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
          mainApp, &MainApplication::updateShapeTransformH);

  connect(buttonLock, &QToolButton::toggled, mainApp, &MainApplication::updateShapeScaleLock);

  connect(mainApp, &MainApplication::changeTransformEnable, [=](bool enable) {
    ui->actionHFlip->setEnabled(enable);
    ui->actionVFlip->setEnabled(enable);
    cutAction_->setEnabled(enable);
    copyAction_->setEnabled(enable);
    duplicateAction_->setEnabled(enable);
    deleteAction_->setEnabled(enable);
    transform_panel_->changeTransformEnable(enable);
    doubleSpinBoxX->setEnabled(enable);
    doubleSpinBoxY->setEnabled(enable);
    doubleSpinBoxRotation->setEnabled(enable);
    doubleSpinBoxWidth->setEnabled(enable);
    doubleSpinBoxHeight->setEnabled(enable);
    buttonLock->setEnabled(enable);
  });
  buttonLock->setChecked(mainApp->isShapeScaleLocked());
}

/*
void MainWindow::setToolbarImage() {
  auto gradientSwitch = new QCheckBox(ui->toolBarImage1);
  auto gradientThresholdSlider = new QSlider(Qt::Horizontal, ui->toolBarImage1);
  auto thresholdLabel = new QLabel(ui->toolBarImage1);
  auto styleHBoxLayout = new QVBoxLayout(ui->toolBarImage1);
  ui->toolBarImage1->setLayout(styleHBoxLayout);
  thresholdLabel->setText(tr("Gradient Treshold"));
  gradientSwitch->setCheckState(Qt::Checked);
  gradientSwitch->setText(tr("Gradient Switch"));
  gradientThresholdSlider->setValue(128);
  ui->toolBarImage1->addWidget(gradientSwitch);
  ui->toolBarImage1->addWidget(thresholdLabel);
  ui->toolBarImage1->addWidget(gradientThresholdSlider);
  auto checkbox_event = QOverload<int>::of(&QCheckBox::stateChanged);
  auto slider_event = QOverload<int>::of(&QSlider::valueChanged);

  //connect(gradientSwitch, checkbox_event, ..., ...);

  connect(gradientThresholdSlider, slider_event, [=](int threshold_value) {

  });

}
*/

void MainWindow::setModeBlock() {
  mode_block_ = new QPushButton(ui->quickWidget);
  mode_block_->setGeometry(ui->quickWidget->geometry().left() + 20, ui->quickWidget->geometry().top() + 15, 100, 30);
  mode_block_->setStyleSheet("QPushButton { border: none; } QPushButton::hover { border: none; background-color: transparent }");
}

void MainWindow::setScaleBlock() {
  scale_block_ = new QPushButton("100%", ui->quickWidget);
  minusBtn_ = new QToolButton(ui->quickWidget);
  plusBtn_ = new QToolButton(ui->quickWidget);
  scale_block_->setGeometry(ui->quickWidget->geometry().left() + 60, this->size().height() - 150, 50, 30);
  scale_block_->setStyleSheet("QPushButton { border: none; } QPushButton::hover { border: none; background-color: transparent }");
  minusBtn_->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-minus.png" : ":/resources/images/icon-minus.png"));
  minusBtn_->setGeometry(ui->quickWidget->geometry().left() + 30, this->size().height() - 150, 40, 30);
  minusBtn_->setStyleSheet("QToolButton { border: none; } QToolButton::hover { border: none; background-color: transparent }");
  plusBtn_->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-plus.png" : ":/resources/images/icon-plus.png"));
  plusBtn_->setGeometry(ui->quickWidget->geometry().left() + 100, this->size().height() - 150, 40, 30);
  plusBtn_->setStyleSheet("QToolButton { border: none; } QToolButton::hover { border: none; background-color: transparent }");

  popScaleMenu_ = new QMenu(scale_block_);
  // Add QActions for context menu
  QAction* scaleFitToScreenAction_ = popScaleMenu_->addAction(tr("Fit to Screen"));
  QAction* scale25Action_ = popScaleMenu_->addAction("25 %");
  QAction* scale50Action_ =  popScaleMenu_->addAction("50 %");
  QAction* scale75Action_ =  popScaleMenu_->addAction("75 %");
  QAction* scale100Action_ =  popScaleMenu_->addAction("100 %");
  QAction* scale150Action_ = popScaleMenu_->addAction("150 %");
  QAction* scale200Action_ = popScaleMenu_->addAction("200 %");

  scale_block_->addAction(scale25Action_);
  scale_block_->addAction(scale50Action_);
  scale_block_->addAction(scale75Action_);
  scale_block_->addAction(scale100Action_);
  scale_block_->addAction(scale150Action_);
  scale_block_->addAction(scale200Action_);

  scale_block_->connect(scaleFitToScreenAction_, &QAction::triggered, [=]() {
    qreal orig_scale = canvas_->document().scale();
    canvas_->document().setScale(qMin((canvas_->width() - 40)/canvas_->document().width(), (canvas_->height() - 40)/canvas_->document().height()));
    QPointF center_pos = QPointF(canvas_->document().width()/2*canvas_->document().scale(), canvas_->document().height()/2*canvas_->document().scale());
    QPointF new_scroll = (QPointF(canvas_->width()/2 + 10, canvas_->height()/2 + 10) - center_pos);
    // Restrict the scroll range (might not be necessary)
    QPointF top_left_bound = canvas_->getTopLeftScrollBoundary();
    QPointF bottom_right_bound = canvas_->getBottomRightScrollBoundary();
    if (center_pos.x() > 0 && new_scroll.x() > top_left_bound.x()) {
      new_scroll.setX(top_left_bound.x());
    } else if (center_pos.x() < 0 && new_scroll.x() < bottom_right_bound.x()) {
      new_scroll.setX(bottom_right_bound.x());
    }
    if (center_pos.y() > 0 && new_scroll.y() > top_left_bound.y()) {
      new_scroll.setY(top_left_bound.y());
    } else if (center_pos.y() < 0 && new_scroll.y() < bottom_right_bound.y()) {
      new_scroll.setY(bottom_right_bound.y());
    }

    scale_block_->setGeometry(ui->quickWidget->geometry().left() + 60, this->size().height() - 150, 50, 30);
    minusBtn_->setGeometry(ui->quickWidget->geometry().left() + 30, this->size().height() - 150, 40, 30);
    plusBtn_->setGeometry(ui->quickWidget->geometry().left() + 100, this->size().height() - 150, 40, 30);
    canvas_->document().setScroll(new_scroll);
  });
  scale_block_->connect(scale25Action_, &QAction::triggered, [=]() {
    canvas_->setScaleWithCenter(0.25);
  });
  scale_block_->connect(scale50Action_, &QAction::triggered, [=]() {
    canvas_->setScaleWithCenter(0.5);
  });
  scale_block_->connect(scale75Action_, &QAction::triggered, [=]() {
    canvas_->setScaleWithCenter(0.75);
  });
  scale_block_->connect(scale100Action_, &QAction::triggered, [=]() {
    canvas_->setScaleWithCenter(1);
  });
  scale_block_->connect(scale150Action_, &QAction::triggered, [=]() {
    canvas_->setScaleWithCenter(1.5);
  });
  scale_block_->connect(scale200Action_, &QAction::triggered, [=]() {
    canvas_->setScaleWithCenter(2);
  });

  scale_block_->setContextMenuPolicy(Qt::CustomContextMenu);

  connect(minusBtn_, &QAbstractButton::clicked, this, &MainWindow::onScaleMinusClicked);
  connect(plusBtn_, &QAbstractButton::clicked, this, &MainWindow::onScalePlusClicked);
  connect(scale_block_, &QAbstractButton::clicked, [=]() {
    if(popScaleMenu_){
      popScaleMenu_->exec(QCursor::pos());
    }
  });
}

void MainWindow::showCanvasPopMenu() {
  if(popMenu_){
      popMenu_->exec(QCursor::pos());
  }
}

void MainWindow::showJoggingPanel() {
  QTimer::singleShot(0, [=]() {
    if(ui->joggingDock->isVisible()) {
      ui->joggingDock->hide();
    } else {
      ui->joggingDock->show();
    }
  });
}

/**
 * @brief Generate gcode from canvas and insert into gcode player (gcode editor)
 */
bool MainWindow::generateGcode() {
  auto gen_gcode = std::make_shared<GCodeGenerator>(mainApp->getMachineParam(), mainApp->isRotaryMode());
  QProgressDialog progress_dialog(tr("Generating GCode..."),
                                   tr("Cancel"),
                                   0,
                                   101, this);
  progress_dialog.setWindowModality(Qt::WindowModal);
  progress_dialog.show();
  progress_dialog.activateWindow();
  progress_dialog.raise();
  QTransform move_translate = calculateTranslate();
  ToolpathExporter exporter(gen_gcode.get(),
      canvas_->document().settings().dpmm(),
      mainApp->getTravelSpeed(),
      end_point_,
      ToolpathExporter::PaddingType::kFixedPadding,
      move_translate);
  exporter.setSortRule(mainApp->getPathSort());
  exporter.setWorkAreaSize(QRectF(0,0,canvas_->document().width() / 10, canvas_->document().height() / 10));
  if ( true != exporter.convertStack(canvas_->document().layers(), mainApp->isHighSpeedMode(), 
                                    mainApp->getStartWithHome(), &progress_dialog)) {
    return false; // canceled
  }
  if (exporter.isExceedingBoundary()) {
    QMessageBox msgbox;
    msgbox.setText(tr("Warning"));
    msgbox.setInformativeText(tr("Some items aren't placed fully inside the working area."));
    msgbox.exec();
  }
  if (mainApp->isRotaryMode() && canvas_->calculateShapeBoundary().height() > canvas_->document().height()) {
    QMessageBox msgbox;
    msgbox.setText(tr("Warning"));
    msgbox.setInformativeText(tr("Some items maybe overlap in rotary mode."));
    msgbox.exec();
  }
  
  gcode_panel_->setGCode(QString::fromStdString(gen_gcode->toString()));
  progress_dialog.setValue(progress_dialog.maximum());

  return true;
}

void MainWindow::genPreviewWindow() {
  // Draw preview
  auto preview_path_generator = std::make_shared<PreviewGenerator>(mainApp->getMachineParam(), mainApp->isRotaryMode());
  QTransform move_translate = calculateTranslate();
  ToolpathExporter preview_exporter(preview_path_generator.get(),
                                    canvas_->document().settings().dpmm(),
                                    mainApp->getTravelSpeed(),
                                    end_point_,
                                    ToolpathExporter::PaddingType::kFixedPadding,
                                    move_translate);
  preview_exporter.setSortRule(mainApp->getPathSort());
  preview_exporter.setWorkAreaSize(QRectF(0,0,canvas_->document().width() / 10, canvas_->document().height() / 10));

  QProgressDialog progress_dialog(tr("Exporting toolpath..."),
                                   tr("Cancel"),
                                   0,
                                   101, this);
  progress_dialog.setWindowModality(Qt::WindowModal);
  progress_dialog.show();
  progress_dialog.activateWindow();
  progress_dialog.raise();

  if ( true != preview_exporter.convertStack(canvas_->document().layers(), mainApp->isHighSpeedMode(), 
                                            mainApp->getStartWithHome(), &progress_dialog)) {
    return; // canceled
  }
  progress_dialog.setValue(progress_dialog.maximum());
  QCoreApplication::processEvents();
  
  // Prepare GCodes
  if (generateGcode() == false) {
    return; // canceled
  }

  // Prepare total required time
  try {
    auto gcode_list = gcode_panel_->getGCode().split('\n');
    auto progress_dialog = new QProgressDialog(
      tr("Estimating task time..."),  
      tr("Cancel"), 
      0, gcode_list.size()-1, 
      this);
    auto timestamp_list = MachineJob::calcRequiredTime(gcode_list, progress_dialog);
    Timestamp last_gcode_timestamp{0, 0};
    if (!timestamp_list.empty()) {
      last_gcode_timestamp = timestamp_list.last();
    }

    double height_scale = 1;
    if(mainApp->isRotaryMode()) height_scale = mainApp->getRotaryScale();
    PreviewWindow *pw = new PreviewWindow(this,
                                          QRectF(0,0,canvas_->document().width() / 10, canvas_->document().height() / 10),
                                          height_scale);
    pw->setPreviewPath(preview_path_generator);
    pw->setRequiredTime(last_gcode_timestamp);
    pw->show();
    pw->activateWindow();
    pw->raise();
  } catch (...) {
    // Terminated
    return;
  }

}

/**
 * @brief Create and launch a new Job,
 *        Attach control panel (gcode panel & job dashboard) to the active job
 *        Currently, support gcode job only
 */
void MainWindow::onStartNewJob() {
  auto gcode_list = gcode_panel_->getGCode().split('\n');
  JobDashboardDialog* job_dialog = qobject_cast<JobDashboardDialog*>(sender());
  if (job_dialog != NULL) {
    // Job launched from job dashboard -> with preview
    auto progress_dialog = new QProgressDialog(
        tr("Estimating task time..."),  
        tr("Cancel"), 
        0, gcode_list.size()-1, 
        qobject_cast<QWidget*>(job_dialog));
    if (true == active_machine.createGCodeJob(gcode_list, 
                                              job_dialog->getPreview(),
                                              progress_dialog)) {
      gcode_panel_->attachJob(active_machine.getJobExecutor());
      job_dashboard_->attachJob(active_machine.getJobExecutor());
      active_machine.startJob();
    }
  } else {
    // Job launched from other component, e.g. gcode panel -> no preview (i.e. no job dashboard)
    auto progress_dialog = new QProgressDialog(
        tr("Estimating task time..."),  
        tr("Cancel"), 
        0, gcode_list.size()-1, 
        qobject_cast<QWidget*>(this));
    if (true == active_machine.createGCodeJob(gcode_list, progress_dialog)) {
      gcode_panel_->attachJob(active_machine.getJobExecutor());
      active_machine.startJob();
    }
  }

}

void MainWindow::onStopJob() {
  active_machine.stopJob();
  active_machine.syncPosition();
}

void MainWindow::onPauseJob() {
  active_machine.pauseJob();
}

void MainWindow::onResumeJob() {
  active_machine.resumeJob();
}

void MainWindow::syncJobState(Executor::State new_state) {
  switch (new_state) {
    case Executor::State::kIdle:
      ui->actionFrame->setEnabled(true);
      jogging_panel_->setControlEnable(true);
      laser_panel_->setControlEnable(true);
      break;
    case Executor::State::kRunning:
      ui->actionFrame->setEnabled(false);
      jogging_panel_->setControlEnable(false);
      laser_panel_->setControlEnable(false);
      break;
    case Executor::State::kPaused:
      ui->actionFrame->setEnabled(false);
      jogging_panel_->setControlEnable(false);
      laser_panel_->setControlEnable(false);
      break;
    case Executor::State::kCompleted:
      ui->actionFrame->setEnabled(true);
      jogging_panel_->setControlEnable(true);
      laser_panel_->setControlEnable(true);
      break;
    case Executor::State::kStopped:
      ui->actionFrame->setEnabled(true);
      jogging_panel_->setControlEnable(true);
      laser_panel_->setControlEnable(true);
      break;
    default:
      break;
  }
}

void MainWindow::jobDashboardFinish(int result) {
  job_dashboard_exist_ = false;
}

void MainWindow::updateTitle(bool file_modified) {
  setWindowModified(file_modified);
  if(file_modified) {
    setWindowTitle(current_filename_ + "* - Swiftray");
  }
  else {
    setWindowTitle(current_filename_ + " - Swiftray");
  }
}

/**
 * @brief 
 * 
 * @param power 0: turn off laser
 *              otherwise, represent percentage of laser emission (0-100%)
 */
void MainWindow::laser(qreal power_percent) {
  QStringList gcode_list;
  if (power_percent == 0) {
    qInfo() << "laser off!";
    gcode_list.push_back("G1S0");
    gcode_list.push_back("M5");
  } else {
    qInfo() << "laser on!";
    gcode_list.push_back("M3");
    gcode_list.push_back("G1F1000");
    // TODO: Convert percent to S value based on machine settings
    //       We currently assume 100% = S1000 here
    gcode_list.push_back(QString("G1S") + QString::number(qRound(10*power_percent)));
  }
  if (true == active_machine.createGCodeJob(gcode_list, nullptr)) {
    gcode_panel_->attachJob(active_machine.getJobExecutor());
    active_machine.startJob();
  }
}

/**
 * @brief Emit a short laser pulse
 * 
 * @param power_percentage 0-100 (%)
 */
void MainWindow::laserPulse(qreal power_percentage) {
  qInfo() << "laser pulse!";
  QStringList gcode_list;
  if (power_percentage > 0) {
    // TODO: Use G04Pxxx to dwell for a given time duration?
    gcode_list.push_back("M5");
    gcode_list.push_back("G91");
    // TODO: Convert from percentage to S value based on machine settings
    //       We currently assumet 100% = S1000 here
    gcode_list.push_back(QString("M3S") + QString::number(qRound(power_percentage * 10)));
    gcode_list.push_back(QString("G1F1200S") + QString::number(qRound(power_percentage * 10)));
    gcode_list.push_back("G1X0Y0");
    gcode_list.push_back("G1F1200S0");
    gcode_list.push_back("G1X0Y0");
    gcode_list.push_back("G90");
  } else {
    gcode_list.push_back("M5");
  }
  if (true == active_machine.createGCodeJob(gcode_list, nullptr)) {
    gcode_panel_->attachJob(active_machine.getJobExecutor());
    active_machine.startJob();
  }
}

void MainWindow::home() {
  QStringList gcode_list;
  // TODO: Determine Job based on motion controller
  //       Send GrblHomeCmd() instead of gcode cmd for grbl controller
  //       Send XXXHomeCmd() for other controller
  gcode_list.push_back("$H");
  if (true == active_machine.createGCodeJob(gcode_list, nullptr)) {
    gcode_panel_->attachJob(active_machine.getJobExecutor());
    active_machine.startJob();
  }
}

void MainWindow::moveRelatively(qreal x, qreal y, qreal feedrate) {
  if (true == active_machine.createJoggingRelativeJob(x, y, 0, feedrate)) 
  {
    gcode_panel_->attachJob(active_machine.getJobExecutor());
    active_machine.startJob();
  }
}

void MainWindow::moveAbsolutely(std::tuple<qreal, qreal, qreal> pos, 
                                qreal feedrate) {
  if (true == active_machine.createJoggingAbsoluteJob(pos, feedrate)) 
  {
    gcode_panel_->attachJob(active_machine.getJobExecutor());
    active_machine.startJob();
  }
}

void MainWindow::moveToEdge(int edge_id, qreal feedrate) {
  if (true == active_machine.createJoggingEdgeJob(edge_id, feedrate)) 
  {
    gcode_panel_->attachJob(active_machine.getJobExecutor());
    active_machine.startJob();
  }
}

void MainWindow::moveToCorner(int corner_id, qreal feedrate) {
  if (true == active_machine.createJoggingCornerJob(corner_id, feedrate)) 
  {
    gcode_panel_->attachJob(active_machine.getJobExecutor());
    active_machine.startJob();
  }
}

void MainWindow::setCustomOrigin(std::tuple<qreal, qreal, qreal> custom_origin) {
  canvas_->setUserOrigin(QPointF(std::get<0>(custom_origin), std::get<1>(custom_origin)));
  active_machine.setCustomOrigin(custom_origin);
}

void MainWindow::moveToCustomOrigin() {
  bool result = false;
  if (mainApp->isRotaryMode()) { // Only move X pos
    result = active_machine.createJoggingXAbsoluteJob(active_machine.getCustomOrigin(), mainApp->getTravelSpeed()*60);// mm/s to mm/min
  } else {
    result = active_machine.createJoggingAbsoluteJob(active_machine.getCustomOrigin(), mainApp->getTravelSpeed()*60);// mm/s to mm/min
  }
  if (result == true)
  {
    gcode_panel_->attachJob(active_machine.getJobExecutor());
    active_machine.startJob();
  }
}

void MainWindow::testRotary(QRectF bbox, char rotary_axis, qreal feedrate, double framing_power) {
  if (true == active_machine.createRotaryTestJob(bbox, rotary_axis, feedrate, framing_power)) 
  {
    gcode_panel_->attachJob(active_machine.getJobExecutor());
    active_machine.startJob();
  }
}

void MainWindow::machinePositionCached(std::tuple<qreal, qreal, qreal> target_pos) {
  Q_EMIT MainWindow::positionCached(target_pos);
  canvas_->updateCurrentPosition(target_pos);
}

void MainWindow::machineDisconnected() {
  Q_EMIT MainWindow::activeMachineDisconnected();
  ui->actionConnect->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-unlink.png" : ":/resources/images/icon-unlink.png"));
  ui->actionFrame->setEnabled(false);
  jogging_panel_->setControlEnable(false);
  laser_panel_->setControlEnable(false);
}

void MainWindow::updateCanvasSize(QSize new_size) {
  canvas_->document().setWidth(new_size.width() * 10);
  canvas_->document().setHeight(new_size.height() * 10);
  canvas_->resize();
}

#if defined(HAVE_SOFTWARE_UPDATE) && defined(Q_OS_WIN)
void MainWindow::softwareUpdateRequested() {
  // Check if we can close the app and update
  // 1. Require user to save unsaved changes
  // 2. TODO: check if task is still running
  if ( ! handleUnsavedChange()) {
    mainApp->rejectSoftwareUpdate();
  }
}
#endif
