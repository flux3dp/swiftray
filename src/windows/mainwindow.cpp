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
  updateSelections();
  showWelcomeDialog();
  setScaleBlock();
  setConnectionToolBar();
}

void MainWindow::loadSettings() {
  #ifdef Q_OS_MACOS
    QSettings settings;
    restoreGeometry(settings.value("window/geometry").toByteArray());
    if (!restoreState(settings.value("window/windowState").toByteArray())) {
      QSettings classic_settings(":/classicUI.ini", QSettings::IniFormat);
      restoreState(classic_settings.value("window/windowState").toByteArray());
    };
  #else
    QSettings settings(":/classicUI.ini", QSettings::IniFormat);
    restoreState(settings.value("window/windowState").toByteArray());
  #endif
  QString current_machine = doc_panel_->getMachineName();
  std::size_t found = current_machine.toStdString().find("Lazervida");
  if(found!=std::string::npos) {
    is_high_speed_mode_ = true;
    preferences_window_->setSpeedMode(is_high_speed_mode_);
  }
  else {
    is_high_speed_mode_ = false;
    preferences_window_->setSpeedMode(is_high_speed_mode_);
  }
  is_high_speed_mode_ = preferences_window_->isHighSpeedMode();
  setWindowModified(false);
  setWindowFilePath(FilePathSettings::getDefaultFilePath());
  setWindowTitle(tr("Untitled") + " - Swiftray");
  current_filename_ = tr("Untitled");

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
      .append(std::to_string(VERSION_MINOR))
      .append(std::to_string(VERSION_BUILD))
      .append(VERSION_SUFFIX)
      .c_str()
  );
  sentry_options_set_require_user_consent(options_, true);
  sentry_init(options_);
  QSettings privacy_settings;
  QVariant upload_code = privacy_settings.value("window/upload", 0);
  is_upload_enable_ = upload_code.toBool();
  if(is_upload_enable_) {
    sentry_user_consent_give();
  }
  else {
    sentry_user_consent_revoke();
  }
#endif
  preferences_window_->setUpload(is_upload_enable_);
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
  connect(ui->quickWidget, &CanvasWidget::dropFile,[=](QPoint point, QString filename) {
    QFile file(filename);

    if (file.open(QFile::ReadOnly)) {
      // Update default file path
      QFileInfo file_info{filename};
      FilePathSettings::setDefaultFilePath(file_info.absoluteDir().absolutePath());

      QByteArray data = file.readAll();
      qInfo() << "File size:" << data.size();

      if (filename.endsWith(".bb")) {
        if ( ! handleUnsavedChange()) {
          return;
        }
        QDataStream stream(data);
        DocumentSerializer ds(stream);
        canvas_->setDocument(ds.deserializeDocument());
        canvas_->document().setCurrentFile(filename);
        canvas_->emitAllChanges();
        emit canvas_->selectionsChanged();
        current_filename_ = QFileInfo(filename).baseName();
        setWindowFilePath(filename);
        setWindowTitle(current_filename_ + " - Swiftray");
      } else if (filename.endsWith(".svg")) {
        canvas_->loadSVG(filename);
        // canvas_->loadSVG(data);
        double scale = 3.0 / 8.5 * 10;
        QPointF paste_shift(canvas_->document().getCanvasCoord(point));
        canvas_->transformControl().updateTransform(paste_shift.x(), paste_shift.y(), r_, w_ * scale, h_ * scale);
      }  else if (filename.endsWith(".dxf")) {
        canvas_->loadDXF(filename);
        QPointF paste_shift(canvas_->document().getCanvasCoord(point));
        canvas_->transformControl().updateTransform(paste_shift.x(), paste_shift.y(), r_, w_ * 100, h_ * 100);
      } else {
        importImage(filename);
        QPointF paste_shift(canvas_->document().getCanvasCoord(point));
        canvas_->transformControl().updateTransform(paste_shift.x(), paste_shift.y(), r_, w_ * 10, h_ * 10);
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

void MainWindow::actionStart() {
  // NOTE: unselect all to show the correct shape colors. Otherwise, the selected shapes will be in blue color.
  canvas_->document().execute(Commands::Select(&canvas_->document(), {}));

  if (!serial_port.isOpen()) {
    QMessageBox msgbox;
    msgbox.setText(tr("Serial Port Error"));
    msgbox.setInformativeText(tr("Please connect to serial port first"));
    msgbox.exec();
    return;
  }
  // Prepare GCodes
  if (generateGcode() == false) {
    return;
  }
  // Prepare total required time
  try {
    auto gcode_list = gcode_player_->getGCode().split('\n');
    auto progress_dialog = new QProgressDialog(
      tr("Estimating task time..."),  
      tr("Cancel"), 
      0, gcode_list.size() - 1, 
      this);
    auto timestamp_list = GrblJob::calcRequiredTime(gcode_list, progress_dialog);
    QTime total_required_time = QTime{0, 0};
    if (!timestamp_list.empty()) {
      total_required_time = timestamp_list.last();
    }
    // Prepare canvas scene pixmap
    QPixmap canvas_pixmap{static_cast<int>(canvas_->document().width()), static_cast<int>(canvas_->document().height())};
    canvas_pixmap.fill(Qt::white);
    auto painter = std::make_unique<QPainter>(&canvas_pixmap);
    canvas_->document().paint(painter.get());

    job_dashboard_ = new JobDashboardDialog(total_required_time, canvas_pixmap, this);
    connect(job_dashboard_, &JobDashboardDialog::startBtnClicked, this, &MainWindow::onStartNewJob);
    connect(job_dashboard_, &JobDashboardDialog::pauseBtnClicked, this, &MainWindow::onPauseJob);
    connect(job_dashboard_, &JobDashboardDialog::resumeBtnClicked, this, &MainWindow::onResumeJob);
    connect(job_dashboard_, &JobDashboardDialog::stopBtnClicked, this, &MainWindow::onStopJob);
    connect(job_dashboard_, &JobDashboardDialog::jobStatusReport, this, &MainWindow::setJobStatus);
    connect(job_dashboard_, &JobDashboardDialog::finished, this, &MainWindow::jobDashboardFinish);
    if (jobs_.length() > 0 && (jobs_.last()->isRunning() || jobs_.last()->isPaused())) {
      job_dashboard_->attachJob(jobs_.last());
    }
    job_dashboard_->show();
    job_dashboard_exist_ = true;
  } catch (...) {
    // Terminated
    return;
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
  emit canvas_->selectionsChanged();
  setWindowModified(false);
  setWindowTitle(tr("Untitled") + " - Swiftray");
  current_filename_ = tr("Untitled");
}

void MainWindow::onScalePlusClicked() {
  canvas_->setScaleWithCenter(qreal(qRound((canvas_->document().scale() + 0.005)*100))/100);
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
                                                   tr("Files (*.bb *.bvg *.svg *.png *.jpg *.jpeg *.bmp *.dxf)"));

  if (!QFile::exists(file_name))
    return;

  QFile file(file_name);

  if (file.open(QFile::ReadOnly)) {
    // Update default file path
    QFileInfo file_info{file_name};
    FilePathSettings::setDefaultFilePath(file_info.absoluteDir().absolutePath());

    QByteArray data = file.readAll();
    qInfo() << "File size:" << data.size();

    if (file_name.endsWith(".bb")) {
      QDataStream stream(data);
      DocumentSerializer ds(stream);
      canvas_->setDocument(ds.deserializeDocument());
      canvas_->document().setCurrentFile(file_name);
      canvas_->emitAllChanges();
      emit canvas_->selectionsChanged();
      current_filename_ = QFileInfo(file_name).baseName();
      setWindowFilePath(file_name);
      setWindowTitle(current_filename_ + " - Swiftray");
    } else if (file_name.endsWith(".svg")) {
      canvas_->loadSVG(file_name);
      // canvas_->loadSVG(data);
    } else if (file_name.endsWith(".dxf")) {
      canvas_->loadDXF(file_name);
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
    emit canvas_->selectionsChanged();
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
    emit canvas_->selectionsChanged();
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
    emit canvas_->selectionsChanged();
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
  if (canvas_->document().activeLayer()->isLocked()) {
    emit canvas_->modeChanged();
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
                                                   tr("Image Files (*.png *.jpg *.jpeg *.svg *.bmp *.dxf)"));

  if (!QFile::exists(file_name))
    return;

  QFileInfo file_info{file_name};
  FilePathSettings::setDefaultFilePath(file_info.absoluteDir().absolutePath());

  if (file_name.endsWith(".svg")) {
    canvas_->loadSVG(file_name);
  } else if (file_name.endsWith(".dxf")) {
    canvas_->loadDXF(file_name);
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
  auto gen_gcode = std::make_shared<GCodeGenerator>(doc_panel_->currentMachine());
  QProgressDialog progress_dialog(tr("Generating GCode..."),
                                   tr("Cancel"),
                                   0,
                                   100, this);
  progress_dialog.setWindowModality(Qt::WindowModal);
  progress_dialog.show();

  ToolpathExporter exporter(gen_gcode.get(),
      canvas_->document().settings().dpmm(), 
      ToolpathExporter::PaddingType::kFixedPadding);
  exporter.setWorkAreaSize(QSizeF{canvas_->document().width() / 10, canvas_->document().height() / 10}); // TODO: Set machine work area in unit of mm
  if ( true != exporter.convertStack(canvas_->document().layers(), is_high_speed_mode_, &progress_dialog)) {
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
    gcode_player_->setGCode(stream.readAll());
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

void MainWindow::updateSelections() {
  QList<ShapePtr> &items = canvas_->document().selections();
  bool all_group = !items.empty();
  bool all_path = !items.empty();
  bool all_image = !items.empty();
  bool all_geometry = !items.empty();
  bool all_text = !items.empty();

  for (auto &shape : canvas_->document().selections()) {
    if (shape->type() != Shape::Type::Group) all_group = false;

    if (shape->type() != Shape::Type::Path && shape->type() != Shape::Type::Text) all_path = false;
    if (shape->type() != Shape::Type::Path) all_geometry = false;
    if (shape->type() != Shape::Type::Bitmap) all_image = false;
    if (shape->type() != Shape::Type::Text) all_text = false;
  }

  cutAction_->setEnabled(items.size() > 0);
  copyAction_->setEnabled(items.size() > 0);
  duplicateAction_->setEnabled(items.size() > 0);
  deleteAction_->setEnabled(items.size() > 0);
  groupAction_->setEnabled(items.size() > 1);
  ungroupAction_->setEnabled(all_group);

  ui->actionGroup->setEnabled(items.size() > 1);
  ui->actionGroupMenu->setEnabled(items.size() > 1);
  ui->actionUngroup->setEnabled(all_group);
  ui->actionUngroupMenu->setEnabled(all_group);
  ui->actionUnion->setEnabled(items.size() > 1 && all_path); // Union can be done with the shape itself if it contains sub polygons
  ui->actionSubtract->setEnabled(items.size() == 2 && all_path);
  ui->actionDiff->setEnabled(items.size() == 2 && all_path);
  ui->actionIntersect->setEnabled(items.size() == 2 && all_path);
  ui->actionHFlip->setEnabled(!items.empty());
  ui->actionVFlip->setEnabled(!items.empty());
  ui->actionAlignVTop->setEnabled(items.size() > 1);
  ui->actionAlignVCenter->setEnabled(items.size() > 1);
  ui->actionAlignVBottom->setEnabled(items.size() > 1);
  ui->actionAlignHLeft->setEnabled(items.size() > 1);
  ui->actionAlignHCenter->setEnabled(items.size() > 1);
  ui->actionAlignHRight->setEnabled(items.size() > 1);
  ui->actionTrace->setEnabled(items.size() == 1 && all_image);
  ui->actionInvert->setEnabled(items.size() == 1 && all_image);
  ui->actionReplace_with->setEnabled(items.size() == 1 && all_image);
  ui->actionCrop->setEnabled(items.size() == 1 && all_image);
  ui->actionPathOffset->setEnabled(all_geometry);
  ui->actionSharpen->setEnabled(items.size() == 1 && all_image);
}

void MainWindow::updateToolbarTransform() {
  canvas()->transformControl().updateTransform(x_ * 10, y_ * 10, r_, w_ * 10, h_ * 10);
  emit toolbarTransformChanged(x_, y_, r_, w_, h_);
}

void MainWindow::loadWidgets() {
  assert(canvas_ != nullptr);
  // TODO (Use event to decouple circular dependency with Mainwindow)
  transform_panel_ = new TransformPanel(ui->objectParamDock, this);
  layer_panel_ = new LayerPanel(ui->layerDockContents, this);
  gcode_player_ = new GCodePlayer(ui->serialPortDock);
  font_panel_ = new FontPanel(ui->fontDock, this);
  image_panel_ = new ImagePanel(ui->imageDock, this);
  doc_panel_ = new DocPanel(ui->documentDock, this);
  jogging_panel_ = new JoggingPanel(ui->joggingDock, this);
  machine_manager_ = new MachineManager(this, this);
  preferences_window_ = new PreferencesWindow(this);
  about_window_ = new AboutWindow(this);
  welcome_dialog_ = new WelcomeDialog(this);
  privacy_window_ = new PrivacyWindow(this);
  rotary_setup_ = new RotarySetup(this);
  ui->joggingDock->setWidget(jogging_panel_);
  ui->objectParamDock->setWidget(transform_panel_);
  ui->serialPortDock->setWidget(gcode_player_);
  ui->fontDock->setWidget(font_panel_);
  ui->imageDock->setWidget(image_panel_);
  ui->layerDock->setWidget(layer_panel_);
  ui->documentDock->setWidget(doc_panel_);
#ifdef Q_OS_IOS
  ui->serialPortDock->setVisible(false);
#endif
  //panel
  ui->actionObjectPanel->setChecked(!ui->objectParamDock->isHidden());
  ui->actionFontPanel->setChecked(!ui->fontDock->isHidden());
  ui->actionImagePanel->setChecked(!ui->imageDock->isHidden());
  ui->actionDocumentPanel->setChecked(!ui->documentDock->isHidden());
  ui->actionLayerPanel->setChecked(!layer_panel_->isHidden());
  ui->actionGCodeViewerPanel->setChecked(!gcode_player_->isHidden());
  ui->actionJoggingPanel->setChecked(!jogging_panel_->isHidden());
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
}

void MainWindow::registerEvents() {
  // Monitor canvas events
  connect(canvas_, &Canvas::modeChanged, this, &MainWindow::updateMode);
  connect(canvas_, &Canvas::selectionsChanged, this, &MainWindow::updateSelections);
  connect(canvas_, &Canvas::scaleChanged, this, &MainWindow::updateScale);
  connect(canvas_, &Canvas::fileModifiedChange, this, &MainWindow::updateTitle);
  connect(canvas_, &Canvas::canvasContextMenuOpened, this, &MainWindow::showCanvasPopMenu);
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
  connect(ui->actionSelect, &QAction::triggered, canvas_, &Canvas::backToSelectMode);
  connect(ui->actionRect, &QAction::triggered, canvas_, &Canvas::editDrawRect);
  connect(ui->actionPolygon, &QAction::triggered, canvas_, &Canvas::editDrawPolygon);
  connect(ui->actionOval, &QAction::triggered, canvas_, &Canvas::editDrawOval);
  connect(ui->actionLine, &QAction::triggered, canvas_, &Canvas::editDrawLine);
  connect(ui->actionPath, &QAction::triggered, canvas_, &Canvas::editDrawPath);
  connect(ui->actionText, &QAction::triggered, canvas_, &Canvas::editDrawText);
  connect(ui->actionPhoto, &QAction::triggered, this, &MainWindow::openImageFile);
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
  connect(ui->actionPreferences, &QAction::triggered, preferences_window_, &PreferencesWindow::show);
  connect(ui->actionAbout, &QAction::triggered, about_window_, &AboutWindow::show);
  connect(ui->actionMachineSettings, &QAction::triggered, machine_manager_, &MachineManager::show);
  connect(ui->actionRotarySetup, &QAction::triggered, rotary_setup_, &RotarySetup::show);
  connect(ui->actionPathOffset, &QAction::triggered, canvas_, &Canvas::genPathOffset);
  connect(ui->actionTrace, &QAction::triggered, canvas_, &Canvas::genImageTrace);
  connect(ui->actionInvert, &QAction::triggered, canvas_, &Canvas::invertImage);
  connect(ui->actionSharpen, &QAction::triggered, canvas_, &Canvas::sharpenImage);
  connect(ui->actionReplace_with, &QAction::triggered, this, &MainWindow::replaceImage);
  connect(ui->actionJogging, &QAction::triggered, this, &MainWindow::showJoggingPanel);
  connect(ui->actionCrop, &QAction::triggered, canvas_, &Canvas::cropImage);
  connect(ui->actionStart, &QAction::triggered, this, &MainWindow::actionStart);
  connect(ui->actionStart_2, &QAction::triggered, this, &MainWindow::actionStart);
  connect(ui->actionFrame, &QAction::triggered, this, [=]() {
    if (!serial_port.isOpen()) {
      QMessageBox msgbox;
      msgbox.setText(tr("Serial Port Error"));
      msgbox.setInformativeText(tr("Please connect to serial port first"));
      msgbox.exec();
      return;
    }
    auto gen_outline_scanning_gcode = std::make_shared<DirtyAreaOutlineGenerator>(doc_panel_->currentMachine());
    ToolpathExporter exporter(gen_outline_scanning_gcode.get(), 
        canvas_->document().settings().dpmm(),
        ToolpathExporter::PaddingType::kNoPadding);
    exporter.setWorkAreaSize(QSizeF{canvas_->document().width() / 10, canvas_->document().height() / 10}); // TODO: Set machine work area in unit of mm
    exporter.convertStack(canvas_->document().layers(), is_high_speed_mode_);

    // TODO: Directly execute without gcode player? (e.g. the same in Jogging panel)
    // Approach 1: use gcode player
    // gcode_player_->setGCode(QString::fromStdString(gen_outline_scanning_gcode->toString()));
    // Approach 2: directy control serial port
    if (!serial_port.isOpen()) {
      return;
    }
    QStringList cmd_list = QString::fromStdString(gen_outline_scanning_gcode->toString()).split("\n");
    for (auto cmd: cmd_list) {
      serial_port.write(cmd + "\n");
      // TODO: Wait for ok?
    }

  });
  connect(machine_manager_, &QDialog::accepted, this, &MainWindow::machineSettingsChanged);

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
  connect(welcome_dialog_, &WelcomeDialog::settingsChanged, [=]() {
    emit machineSettingsChanged();
  });
  connect(welcome_dialog_, &WelcomeDialog::finished, [=](int result) {
    QSettings privacy_settings;
    QVariant newstart_code = privacy_settings.value("window/newstart", 0);
    if(!newstart_code.toInt()) {
      privacy_window_->show();
      privacy_window_->activateWindow();
      privacy_window_->raise();
      privacy_settings.setValue("window/newstart", 1);
    }
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

  connect(gcode_player_, &GCodePlayer::exportGcode, this, &MainWindow::exportGCodeFile);
  connect(gcode_player_, &GCodePlayer::importGcode, this, &MainWindow::importGCodeFile);
  connect(gcode_player_, &GCodePlayer::generateGcode, this, &MainWindow::generateGcode);
  connect(gcode_player_, &GCodePlayer::startBtnClicked, this, &MainWindow::onStartNewJob);
  connect(gcode_player_, &GCodePlayer::pauseBtnClicked, this, &MainWindow::onPauseJob);
  connect(gcode_player_, &GCodePlayer::resumeBtnClicked, this, &MainWindow::onResumeJob);
  connect(gcode_player_, &GCodePlayer::stopBtnClicked, this, &MainWindow::onStopJob);
  connect(gcode_player_, &GCodePlayer::jobStatusReport, this, &MainWindow::setJobStatus);
  connect(&serial_port, &SerialPort::connected, [=]() {
    ui->actionConnect->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-link.png" : ":/resources/images/icon-link.png"));
  });
  connect(&serial_port, &SerialPort::disconnected, [=]() {
    ui->actionConnect->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-unlink.png" : ":/resources/images/icon-unlink.png"));
  });
  connect(preferences_window_, &PreferencesWindow::speedModeChanged, [=](bool is_high_speed) {
    is_high_speed_mode_ = is_high_speed;
  });
  connect(preferences_window_, &PreferencesWindow::privacyUpdate, [=](bool enable_upload) {
    is_upload_enable_ = enable_upload;
#ifdef ENABLE_SENTRY
    if(is_upload_enable_) {
      sentry_user_consent_give();
    }
    else {
      sentry_user_consent_revoke();
    }
#endif
    QSettings settings;
    settings.setValue("window/upload", is_upload_enable_);
  });
  connect(privacy_window_, &PrivacyWindow::privacyUpdate, [=](bool enable_upload) {
    is_upload_enable_ = enable_upload;
#ifdef ENABLE_SENTRY
    if(is_upload_enable_) {
      sentry_user_consent_give();
    }
    else {
      sentry_user_consent_revoke();
    }
#endif
    preferences_window_->setUpload(is_upload_enable_);
    QSettings settings;
    settings.setValue("window/upload", is_upload_enable_);
  });
  connect(doc_panel_, &DocPanel::machineChanged, [=](QString machine_name) {
    std::size_t found = machine_name.toStdString().find("Lazervida");
    if(found!=std::string::npos) {
      is_high_speed_mode_ = true;
      preferences_window_->setSpeedMode(is_high_speed_mode_);
      QMessageBox msgbox;
      msgbox.setText(tr("Alarm"));
      msgbox.setInformativeText(tr("Please confirm that you are using the Lazervida machine."));
      msgbox.exec();
    }
    else {
      is_high_speed_mode_ = false;
      preferences_window_->setSpeedMode(is_high_speed_mode_);
    }
  });
  connect(doc_panel_, &DocPanel::rotaryModeChange, [=](bool is_rotary_mode) {
    is_rotary_mode_ = is_rotary_mode;
    rotary_setup_->setRotaryMode(is_rotary_mode);
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
  connect(rotary_setup_, &RotarySetup::rotaryModeChanged, [=](bool is_rotary_mode) {
    is_rotary_mode_ = is_rotary_mode;
    doc_panel_->setRotaryMode(is_rotary_mode_);
  });
  connect(rotary_setup_, &RotarySetup::mirrorModeChanged, [=](bool is_mirror_mode) {
    is_mirror_mode_ = is_mirror_mode;
  });
  connect(rotary_setup_, &RotarySetup::rotaryAxisChanged, [=](QString rotary_axis) {
    rotary_axis_ = rotary_axis;
  });
}

void MainWindow::closeEvent(QCloseEvent *event) {
  if (!handleUnsavedChange()) {
    event->ignore();
  }
  else{
    QSettings settings;
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


MachineSettings::MachineSet MainWindow::currentMachine() {
  return doc_panel_->currentMachine();
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
    if (serial_port.isOpen()) {
      auto matchIt = std::find_if(
              infos.begin(), infos.end(),
              [](const QSerialPortInfo& info) { return info.portName() == serial_port.portName(); }
      );
      if (matchIt == infos.end()) { // Unplugged -> close opened port
        serial_port.close();
      }
    }

  });
  connect(ui->actionConnect, &QAction::triggered, [=]() {
    if (serial_port.isOpen()) {
      qInfo() << "[SerialPort] Disconnect";
      serial_port.close(); // disconnect
      return;
    }
    QString port_name = portComboBox_->currentText();
    QString baudrate = baudComboBox_->currentText();
    qInfo() << "[SerialPort] Connecting" << port_name << baudrate;
    serial_port.open(port_name, baudrate.toInt());
    if (!serial_port.isOpen()) {
      // Do something?
      return;
    }
  });
  timer->start(4000);
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

  QFont initialFont = QFont(FONT_TYPE, FONT_SIZE, QFont::Bold);

  fontComboBox->setCurrentFont(initialFont);
  doubleSpinBoxLetterSpacing->setValue(initialFont.letterSpacing());
  doubleSpinBoxLineHeight->setValue(1.2);
  spinBoxSize->setValue(initialFont.pointSize());
  boldToolButton->setChecked(initialFont.bold());

  ui->toolBarFont->addWidget(labelSize);
  ui->toolBarFont->addWidget(spinBoxSize);
  ui->toolBarFont->addWidget(labelLineHeight);
  ui->toolBarFont->addWidget(doubleSpinBoxLineHeight);
  ui->toolBarFont->addWidget(labelLetterSpacing);
  ui->toolBarFont->addWidget(doubleSpinBoxLetterSpacing);

  connect(fontComboBox, &QFontComboBox::currentFontChanged, canvas(), &Canvas::setFont);

  connect(fontComboBox, &QFontComboBox::currentFontChanged, [=](QFont selected_font) {
    QFont font = font_panel_->font();
    font.setFamily(selected_font.family());
    font_panel_->setFont(font);
  });

  connect(spinBoxSize, spin_int_event, canvas(), &Canvas::setPointSize);

  connect(spinBoxSize, spin_int_event, font_panel_, &FontPanel::setPointSize);

  connect(doubleSpinBoxLetterSpacing, spin_event, canvas(), &Canvas::setLetterSpacing);

  connect(doubleSpinBoxLetterSpacing, spin_event, font_panel_, &FontPanel::setLetterSpacing);

  connect(doubleSpinBoxLineHeight, spin_event, canvas(), &Canvas::setLineHeight);

  connect(doubleSpinBoxLineHeight, spin_event, font_panel_, &FontPanel::setLineHeight);

  connect(boldToolButton, &QToolButton::toggled, canvas(), &Canvas::setBold);

  connect(boldToolButton, &QToolButton::toggled, font_panel_, &FontPanel::setBold);

  connect(italicToolButton, &QToolButton::toggled, canvas(), &Canvas::setItalic);

  connect(italicToolButton, &QToolButton::toggled, font_panel_, &FontPanel::setItalic);

  connect(underlineToolButton, &QToolButton::toggled, canvas(), &Canvas::setUnderline);

  connect(underlineToolButton, &QToolButton::toggled, font_panel_, &FontPanel::setUnderline);

  connect(font_panel_, &FontPanel::fontChanged, [=](QFont new_font) {
    fontComboBox->setCurrentFont(new_font);
  });

  connect(font_panel_, &FontPanel::fontPointSizeChanged, [=](int value){
    spinBoxSize->setValue(value);
  });

  connect(font_panel_, &FontPanel::fontLetterSpacingChanged, [=](double value){
    doubleSpinBoxLetterSpacing->setValue(value);
  });

  connect(font_panel_, &FontPanel::fontBoldChanged, [=](bool checked){
    boldToolButton->setChecked(checked);
  });

  connect(font_panel_, &FontPanel::fontItalicChanged, [=](bool checked){
    italicToolButton->setChecked(checked);
  });

  connect(font_panel_, &FontPanel::fontUnderlineChanged, [=](bool checked){
    underlineToolButton->setChecked(checked);
  });

  connect(font_panel_, &FontPanel::lineHeightChanged, [=](double line_height) {
    doubleSpinBoxLineHeight->setValue(line_height);
  });

// the ui status is the same to the font panel
  connect(canvas(), &Canvas::selectionsChanged, this, [=]() {
    QFont first_qfont;
    double first_linehight;
    bool has_txt = false;
    bool is_font_changed = false, is_pt_changed = false, is_ls_changed = false, is_linehight_changed = false;
    bool is_bold_changed = false, is_italic_changed = false, is_underline_changed = false;
    for (auto &shape : canvas_->document().selections()) {
      if (shape->type() == ::Shape::Type::Text) {
        auto *t = (TextShape *) shape.get();
        if(!has_txt) {
          first_linehight = t->lineHeight();
          first_qfont = t->font();
          has_txt = true;
        }
        if(has_txt && first_qfont.family() != t->font().family())                is_font_changed = true;
        if(has_txt && first_qfont.pointSize() != t->font().pointSize())          is_pt_changed = true;
        if(has_txt && first_qfont.letterSpacing() != t->font().letterSpacing())  is_ls_changed = true;
        if(has_txt && first_qfont.bold() != t->font().bold())                    is_bold_changed = true;
        if(has_txt && first_qfont.italic() != t->font().italic())                is_italic_changed = true;
        if(has_txt && first_qfont.underline() != t->font().underline())          is_underline_changed = true;
        if(has_txt && first_linehight != t->lineHeight())                        is_linehight_changed = true;
      }
    }
    if(is_font_changed) {
      fontComboBox->blockSignals(true);
      fontComboBox->setCurrentText("");
      fontComboBox->blockSignals(false);
    }
    else if(has_txt) {
      fontComboBox->setCurrentText(first_qfont.family());
    }
    else {
      fontComboBox->setCurrentText(fontComboBox->currentFont().family());
    }
    if(is_pt_changed) {
      spinBoxSize->blockSignals(true);
      spinBoxSize->setSpecialValueText(tr(" "));
      spinBoxSize->setValue(0);
      spinBoxSize->blockSignals(false);
    }
    if(is_ls_changed) {
      doubleSpinBoxLetterSpacing->blockSignals(true);
      doubleSpinBoxLetterSpacing->setSpecialValueText(tr(" "));
      doubleSpinBoxLetterSpacing->setValue(-0.1);
      doubleSpinBoxLetterSpacing->blockSignals(false);
    }
    if(is_bold_changed) {
      boldToolButton->blockSignals(true);
      boldToolButton->setChecked(false);
      boldToolButton->blockSignals(false);
    }
    if(is_italic_changed) {
      italicToolButton->blockSignals(true);
      italicToolButton->setChecked(false);
      italicToolButton->blockSignals(false);
    }
    if(is_underline_changed) {
      underlineToolButton->blockSignals(true);
      underlineToolButton->setChecked(false);
      underlineToolButton->blockSignals(false);
    }
    if(is_linehight_changed) {
      doubleSpinBoxLineHeight->blockSignals(true);
      doubleSpinBoxLineHeight->setSpecialValueText(tr(" "));
      doubleSpinBoxLineHeight->setValue(0);
      doubleSpinBoxLineHeight->blockSignals(false);
    }
  });
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
  doubleSpinBoxRotation->setMaximum(360);
  doubleSpinBoxRotation->setMinimum(0);
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

  auto spin_event = QOverload<double>::of(&QDoubleSpinBox::valueChanged);

  connect(canvas(), &Canvas::transformChanged, [=](qreal x, qreal y, qreal r, qreal w, qreal h) {
    x_ = x/10;
    y_ = y/10;
    r_ = r;
    w_ = w/10;
    h_ = h/10;
    doubleSpinBoxX->blockSignals(true);
    doubleSpinBoxY->blockSignals(true);
    doubleSpinBoxRotation->blockSignals(true);
    doubleSpinBoxWidth->blockSignals(true);
    doubleSpinBoxHeight->blockSignals(true);
    doubleSpinBoxX->setValue(x/10);
    doubleSpinBoxY->setValue(y/10);
    doubleSpinBoxRotation->setValue(r);
    doubleSpinBoxWidth->setValue(w/10);
    doubleSpinBoxHeight->setValue(h/10);
    doubleSpinBoxX->blockSignals(false);
    doubleSpinBoxY->blockSignals(false);
    doubleSpinBoxRotation->blockSignals(false);
    doubleSpinBoxWidth->blockSignals(false);
    doubleSpinBoxHeight->blockSignals(false);
  });

  connect(transform_panel_, &TransformPanel::transformPanelUpdated, [=](double x, double y, double r, double w, double h) {
    x_ = x;
    y_ = y;
    r_ = r;
    w_ = w;
    h_ = h;
    doubleSpinBoxX->setValue(x);
    doubleSpinBoxY->setValue(y);
    doubleSpinBoxRotation->setValue(r);
    doubleSpinBoxWidth->setValue(w);
    doubleSpinBoxHeight->setValue(h);
  });

  connect(doubleSpinBoxX, spin_event, [=]() {
    x_ = doubleSpinBoxX->value();
    updateToolbarTransform();
  });

  connect(doubleSpinBoxY, spin_event, [=]() {
    y_ = doubleSpinBoxY->value();
    updateToolbarTransform();
  });

  connect(doubleSpinBoxRotation, spin_event, [=]() {
    r_ = doubleSpinBoxRotation->value();
    updateToolbarTransform();
  });

  connect(doubleSpinBoxWidth, spin_event, [=]() {
    if (transform_panel_->isScaleLock()) {
      h_ = w_ == 0 ? 0 : h_ * doubleSpinBoxWidth->value() / w_;
    }
    w_ = doubleSpinBoxWidth->value();
    updateToolbarTransform();
  });

  connect(doubleSpinBoxHeight, spin_event, [=]() {
    if (transform_panel_->isScaleLock()) {
      w_ = h_ == 0 ? 0 : w_ * doubleSpinBoxHeight->value() / h_;
    }
    h_ = doubleSpinBoxHeight->value();
    updateToolbarTransform();
  });

  connect(transform_panel_, &TransformPanel::scaleLockToggled, [=](bool scale_locked) {
    buttonLock->setChecked(scale_locked);

    if (scale_locked) {
      buttonLock->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-lock.png" : ":/resources/images/icon-lock.png"));
    } else {
      buttonLock->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-unlock.png" : ":/resources/images/icon-unlock.png"));
    }
  });

  connect(buttonLock, &QToolButton::toggled, [=](bool checked) {
    transform_panel_->setScaleLock(checked);
  });

  //panel
  connect(ui->objectParamDock, &QDockWidget::visibilityChanged, [=](bool is_visible) {
    ui->actionObjectPanel->setChecked(is_visible);
  });
  connect(ui->fontDock, &QDockWidget::visibilityChanged, [=](bool is_visible) {
    ui->actionFontPanel->setChecked(is_visible);
  });
  connect(ui->imageDock, &QDockWidget::visibilityChanged, [=](bool is_visible) {
    ui->actionImagePanel->setChecked(is_visible);
  });
  connect(ui->documentDock, &QDockWidget::visibilityChanged, [=](bool is_visible) {
    ui->actionDocumentPanel->setChecked(is_visible);
  });
  connect(layer_panel_, &LayerPanel::panelShow, [=](bool is_show) {
    ui->actionLayerPanel->setChecked(is_show);
  });
  connect(gcode_player_, &GCodePlayer::panelShow, [=](bool is_show) {
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

void MainWindow::showWelcomeDialog() {
  //if (!MachineSettings().machines().empty()) return;
  QTimer::singleShot(0, [=]() {
    welcome_dialog_->show();
    welcome_dialog_->activateWindow();
    welcome_dialog_->raise();
  });
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
  auto gen_gcode = std::make_shared<GCodeGenerator>(doc_panel_->currentMachine());
  QProgressDialog progress_dialog(tr("Generating GCode..."),
                                   tr("Cancel"),
                                   0,
                                   101, this);
  progress_dialog.setWindowModality(Qt::WindowModal);
  progress_dialog.show();
  ToolpathExporter exporter(gen_gcode.get(),
      canvas_->document().settings().dpmm(),
      ToolpathExporter::PaddingType::kFixedPadding);
  exporter.setWorkAreaSize(QSizeF{canvas_->document().width() / 10, canvas_->document().height() / 10}); // TODO: Set machine work area in unit of mm
  if ( true != exporter.convertStack(canvas_->document().layers(), is_high_speed_mode_, &progress_dialog)) {
    return false; // canceled
  }
  if (exporter.isExceedingBoundary()) {
    QMessageBox msgbox;
    msgbox.setText(tr("Warning"));
    msgbox.setInformativeText(tr("Some items aren't placed fully inside the working area."));
    msgbox.exec();
  }
  
  gcode_player_->setGCode(QString::fromStdString(gen_gcode->toString()));
  progress_dialog.setValue(progress_dialog.maximum());

  return true;
}

void MainWindow::genPreviewWindow() {
  // Draw preview
  auto preview_path_generator = std::make_shared<PreviewGenerator>(doc_panel_->currentMachine());

  ToolpathExporter preview_exporter(preview_path_generator.get(),
                                    canvas_->document().settings().dpmm(),
                                    ToolpathExporter::PaddingType::kFixedPadding);

  preview_exporter.setWorkAreaSize(QSizeF{canvas_->document().width() / 10, canvas_->document().height() / 10});

  QProgressDialog progress_dialog(tr("Exporting toolpath..."),
                                   tr("Cancel"),
                                   0,
                                   101, this);
  progress_dialog.setWindowModality(Qt::WindowModal);
  progress_dialog.show();

  if ( true != preview_exporter.convertStack(canvas_->document().layers(), is_high_speed_mode_, &progress_dialog)) {
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
    auto gcode_list = gcode_player_->getGCode().split('\n');
    auto progress_dialog = new QProgressDialog(
      tr("Estimating task time..."),  
      tr("Cancel"), 
      0, gcode_list.size()-1, 
      this);
    auto timestamp_list = GrblJob::calcRequiredTime(gcode_list, progress_dialog);
    QTime last_gcode_timestamp{0, 0};
    if (!timestamp_list.empty()) {
      last_gcode_timestamp = timestamp_list.last();
    }

    PreviewWindow *pw = new PreviewWindow(this,
                                          canvas_->document().width() / 10,
                                          canvas_->document().height() / 10);
    pw->setPreviewPath(preview_path_generator);
    pw->setRequiredTime(last_gcode_timestamp);
    pw->show();
  } catch (...) {
    // Terminated
    return;
  }

}

/**
 * @brief Generate a new job from Gcodes in gcode editor (gcode player)
 */
void MainWindow::generateJob() {
  // Delete and pop all finished jobs
  if (!jobs_.isEmpty()) {
    qInfo() << "delete finished jobs";
    if (jobs_.last()->thread() != nullptr && jobs_.last()->isFinished()) {
      jobs_.last()->deleteLater();
      jobs_.pop_back();
    }
  }

  // Check whether any job still running
  if (!jobs_.isEmpty()) { // at least one job hasn't finist -> don't start execute
    qInfo() << "Blocked: Some jobs are still running";
    return;
  }
  try {
    auto gcode_list = gcode_player_->getGCode().split('\n');
    auto progress_dialog = new QProgressDialog(
      tr("Estimating task time..."),  
      tr("Cancel"), 
      0, gcode_list.size()-1, 
      job_dashboard_exist_ ? qobject_cast<QWidget*>(job_dashboard_) : qobject_cast<QWidget*>(this));
    QElapsedTimer timer;
    qInfo() << "Start calcRequiredTime";
    timer.start();
    auto timestamp_list = GrblJob::calcRequiredTime(gcode_list, progress_dialog);
    auto job = new GrblJob(this, "", gcode_list);
    job->setTimestampList(timestamp_list);
    qDebug() << "The calcRequiredTime took" << timer.elapsed() << "milliseconds";

    jobs_ << job;
  } catch (...) {
    // Terminated
    return;
  }
}

/**
 * @brief launch a new Serial Job (might be other job in the future)
 */
void MainWindow::onStartNewJob() {
  generateJob();
  if (jobs_.empty()) {
    return;
  }

  if (jobs_.count() != 1 && jobs_.last()->status() != BaseJob::Status::READY) {
    qInfo() << "Blocked: No job is ready to run";
    return;
  }

  gcode_player_->attachJob(jobs_.last());
  if(job_dashboard_exist_) {
    job_dashboard_->attachJob(jobs_.last());
  }
  jobs_.last()->start();
}

void MainWindow::onStopJob() {
  // Delete finished jobs
  for (auto job: jobs_) {
    if (!jobs_.last()->isFinished()) {
      jobs_.last()->stop();
    }
  }
}

void MainWindow::onPauseJob() {
  if (jobs_.isEmpty()) {
    return;
  }
  auto job = jobs_.last();
  job->pause();
}

void MainWindow::onResumeJob() {
  if (jobs_.isEmpty()) {
    return;
  }
  auto job = jobs_.last();
  job->resume();
}

void MainWindow::setJobStatus(BaseJob::Status status) {
  switch (status) {
    case BaseJob::Status::READY:
      ui->actionFrame->setEnabled(true);
      jogging_panel_->setControlEnable(true);
      break;
    case BaseJob::Status::STARTING:
      ui->actionFrame->setEnabled(false);
      jogging_panel_->setControlEnable(false);
      break;
    case BaseJob::Status::RUNNING:
      ui->actionFrame->setEnabled(false);
      jogging_panel_->setControlEnable(false);
      break;
    case BaseJob::Status::PAUSED:
      ui->actionFrame->setEnabled(false);
      jogging_panel_->setControlEnable(false);
      break;
    case BaseJob::Status::FINISHED:
      ui->actionFrame->setEnabled(true);
      jogging_panel_->setControlEnable(true);
      break;
    case BaseJob::Status::ALARM:
      ui->actionFrame->setEnabled(false);
      jogging_panel_->setControlEnable(false);
      break;
    case BaseJob::Status::STOPPED:
    case BaseJob::Status::ALARM_STOPPED:
      ui->actionFrame->setEnabled(true);
      jogging_panel_->setControlEnable(true);
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
