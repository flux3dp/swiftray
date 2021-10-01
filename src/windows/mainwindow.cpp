#include <QDebug>
#include <QFileDialog>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickWidget>
#include <shape/bitmap-shape.h>
#include <widgets/components/canvas-text-edit.h>
#include <windows/mainwindow.h>
#include <windows/osxwindow.h>
#include <windows/preview-window.h>
#include <gcode/toolpath-exporter.h>
#include <gcode/generators/gcode-generator.h>
#include <document-serializer.h>
#include <windows/path-offset-dialog.h>

#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
     QMainWindow(parent),
     ui(new Ui::MainWindow),
     canvas_(nullptr),
     BaseContainer() {
  ui->setupUi(this);
  loadCanvas();
  initializeContainer();
  updateMode();
  updateSelections();
  showWelcomeDialog();
}

void MainWindow::loadSettings() {
  QSettings settings;
  restoreGeometry(settings.value("window/geometry").toByteArray());
  restoreState(settings.value("window/windowState").toByteArray());
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
}

void MainWindow::loadStyles() {
  QFile file(isDarkMode() ?
             ":/styles/swiftray-dark.qss" :
             ":/styles/swiftray-light.qss");
  file.open(QFile::ReadOnly);
  QString styleSheet = QLatin1String(file.readAll());
  setStyleSheet(styleSheet);

  QList<QToolBar *> toolbars = {
       ui->toolBar,
       ui->toolBarAlign,
       ui->toolBarBool,
       ui->toolBarGroup,
       ui->toolBarFlip
  };
  for (QToolBar *toolbar : toolbars) {
    for (QAction *action : toolbar->actions()) {
      auto name = action->objectName().mid(6).toLower();
      action->setIcon(QIcon(
           (isDarkMode() ? ":/images/dark/icon-" : ":/images/icon-") + name
      ));
    }
  }
}

void MainWindow::openFile() {
  QString file_name = QFileDialog::getOpenFileName(this, "Open SVG", ".",
                                                   tr("SVG Files (*.svg);;BVG Files (*.bvg);;Scene Files (*.bb)"));

  if (!QFile::exists(file_name))
    return;

  QFile file(file_name);

  if (file.open(QFile::ReadOnly)) {
    QByteArray data = file.readAll();
    qInfo() << "File size:" << data.size();

    if (file_name.endsWith(".bb")) {
      QDataStream stream(data);
      DocumentSerializer ds(stream);
      canvas_->setDocument(ds.deserializeDocument());
      canvas_->emitAllChanges();
      emit canvas_->selectionsChanged();
    } else {
      canvas_->loadSVG(data);
    }
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
  QString file_name = QFileDialog::getOpenFileName(this, "Open Image", ".", tr("Image Files (*.png *.jpg)"));

  if (!QFile::exists(file_name))
    return;

  QImage image;

  if (image.load(file_name)) {
    qInfo() << "File size:" << image.size();
    canvas_->importImage(image);
  }
}

void MainWindow::imageSelected(const QImage image) {
  QImage my_image = image;
  canvas_->importImage(my_image);
}

void MainWindow::exportGCodeFile() {
  auto gen_gcode = make_shared<GCodeGenerator>(doc_panel_->currentMachine());
  ToolpathExporter exporter(gen_gcode.get());
  exporter.convertStack(canvas_->document().layers());

  QString default_save_dir;
  QSettings settings;
  if ( ! settings.contains("defaultSaveDir")) {
    QStringList desktop_dir = QStandardPaths::standardLocations(
            QStandardPaths::StandardLocation::DesktopLocation);
    settings.setValue("defaultSaveDir", desktop_dir.at(0));
  }
  default_save_dir = settings.value("defaultSaveDir").toString();

  QString filter = tr("GCode Files (*.gcode);; All files (*.*)");
  QString file_name = QFileDialog::getSaveFileName(this,
                                                   tr("Save GCode"),
                                                   default_save_dir + "/" + tr("untitled.gcode"),
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
    settings.setValue("defaultSaveDir", file_info.absoluteDir().absolutePath());
  }
}


void MainWindow::saveFile() {
  QString file_name = QFileDialog::getSaveFileName(this, "Save Image", ".", tr("Scene File (*.bb)"));
  QFile file(file_name);

  if (file.open(QFile::ReadWrite)) {
    QDataStream stream(&file);
    canvas_->save(stream);
    file.close();
    qInfo() << "Saved";
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
  delete ui;
}

void MainWindow::updateMode() {
  for (QAction *action : ui->toolBar->actions()) {
    action->setChecked(false);
  }

  const map<Canvas::Mode, QAction *> actionMap = {
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

void MainWindow::updateSelections() {
  QList<ShapePtr> &items = canvas_->document().selections();
  bool all_group = !items.empty();
  bool all_path = !items.empty();
  bool all_image = !items.empty();
  bool all_geometry = !items.empty();

  for (auto &shape : canvas_->document().selections()) {
    if (shape->type() != Shape::Type::Group) all_group = false;

    if (shape->type() != Shape::Type::Path && shape->type() != Shape::Type::Text) all_path = false;
    if (shape->type() != Shape::Type::Path) all_geometry = false;
    if (shape->type() != Shape::Type::Bitmap) all_image = false;
  }

  ui->actionGroup->setEnabled(items.size() > 1);
  ui->actionUngroup->setEnabled(all_group);
  ui->actionUnion->setEnabled(all_path); // Union can be done with the shape itself if it contains sub polygons
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
  ui->actionPathOffset->setEnabled(all_geometry);
#ifdef Q_OS_MACOS
  setOSXWindowTitleColor(this);
#endif
}

void MainWindow::loadWidgets() {
  assert(canvas_ != nullptr);
  // TODO (Use event to decouple circular dependency with Mainwindow)
  transform_panel_ = new TransformPanel(ui->objectParamDock, this);
  layer_panel_ = new LayerPanel(ui->layerDockContents, this);
  gcode_player_ = new GCodePlayer(ui->serialPortDock);
  font_panel_ = new FontPanel(ui->fontDock, this);
  doc_panel_ = new DocPanel(ui->documentDock, this);
  machine_manager_ = new MachineManager(this);
  preferences_window_ = new PreferencesWindow(this);
  welcome_dialog_ = new WelcomeDialog(this);
  image_trace_dialog_ = new ImageTraceDialog(this);
  ui->objectParamDock->setWidget(transform_panel_);
  ui->serialPortDock->setWidget(gcode_player_);
  ui->fontDock->setWidget(font_panel_);
  ui->layerDock->setWidget(layer_panel_);
  ui->documentDock->setWidget(doc_panel_);
#ifdef Q_OS_IOS
  ui->serialPortDock->setVisible(false);
#endif
}

void MainWindow::registerEvents() {
  // Monitor canvas events
  connect(canvas_, &Canvas::modeChanged, this, &MainWindow::updateMode);
  connect(canvas_, &Canvas::selectionsChanged, this, &MainWindow::updateSelections);
  // Monitor UI events
  connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
  connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFile);
  connect(ui->actionClose, &QAction::triggered, this, &MainWindow::close);
  connect(ui->actionCut, &QAction::triggered, canvas_, &Canvas::editCut);
  connect(ui->actionCopy, &QAction::triggered, canvas_, &Canvas::editCopy);
  connect(ui->actionPaste, &QAction::triggered, canvas_, &Canvas::editPaste);
  connect(ui->actionUndo, &QAction::triggered, canvas_, &Canvas::editUndo);
  connect(ui->actionRedo, &QAction::triggered, canvas_, &Canvas::editRedo);
  connect(ui->actionSelect_All, &QAction::triggered, canvas_, &Canvas::editSelectAll);
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
  connect(ui->actionUngroup, &QAction::triggered, canvas_, &Canvas::editUngroup);
  connect(ui->actionHFlip, &QAction::triggered, canvas_, &Canvas::editHFlip);
  connect(ui->actionVFlip, &QAction::triggered, canvas_, &Canvas::editVFlip);
  connect(ui->actionAlignVTop, &QAction::triggered, canvas_, &Canvas::editAlignVTop);
  connect(ui->actionAlignVCenter, &QAction::triggered, canvas_, &Canvas::editAlignVCenter);
  connect(ui->actionAlignVBottom, &QAction::triggered, canvas_, &Canvas::editAlignVBottom);
  connect(ui->actionAlignHLeft, &QAction::triggered, canvas_, &Canvas::editAlignHLeft);
  connect(ui->actionAlignHCenter, &QAction::triggered, canvas_, &Canvas::editAlignHCenter);
  connect(ui->actionAlignHRight, &QAction::triggered, canvas_, &Canvas::editAlignHRight);
  connect(ui->actionPreferences, &QAction::triggered, preferences_window_, &PreferencesWindow::show);
  connect(ui->actionMachineSettings, &QAction::triggered, machine_manager_, &MachineManager::show);
  connect(ui->actionPathOffset, &QAction::triggered, [=]() {
    QList<ShapePtr> &shapes = canvas_->document().selections();
    PathOffsetDialog *dialog = new PathOffsetDialog();
    // initialize dialog
    for (auto &shape : shapes) {
      auto path_shape_ptr = dynamic_cast<PathShape *>(shape.get());
      auto polygons = path_shape_ptr->path().toSubpathPolygons(shape->transform());
      for (QPolygonF &poly : polygons) {
        dialog->addPath(poly);
      }
    }
    dialog->updatePathOffset();

    // output result
    int dialogRet = dialog->exec();
    if(dialogRet == QDialog::Accepted) {
      QPainterPath p;
      for (auto polygon :dialog->getResult()) {
        p.addPolygon(polygon);
      }
      ShapePtr new_shape = make_shared<PathShape>(p);
      canvas_->document().execute(
              Commands::AddShape(canvas_->document().activeLayer(), new_shape),
              Commands::Select(&(canvas_->document()), {new_shape})
      );
    }
    delete dialog;
  });
  connect(ui->actionTrace, &QAction::triggered, [=]() {
    QList<ShapePtr> &items = canvas_->document().selections();
    Q_ASSERT_X(items.count() == 1, "actionTrace", "MUST only be enabled when single item is selected");
    Q_ASSERT_X(items.at(0)->type() == Shape::Type::Bitmap, "actionTrace", "MUST only be enabled when an image is selected");

    ShapePtr selected_img_shape = items.at(0);
    BitmapShape * bitmap = static_cast<BitmapShape *>(selected_img_shape.get());
    this->image_trace_dialog_->reset();
    this->image_trace_dialog_->loadImage(bitmap->image());
    int dialogRet = this->image_trace_dialog_->exec();
    if(dialogRet == QDialog::Accepted) {
      // Add trace contours to canvas
      ShapePtr new_shape = make_shared<PathShape>(this->image_trace_dialog_->getTrace());
      QTransform offset = new_shape->transform();
      offset.translate(bitmap->x(), bitmap->y()); // offset of center of image
      new_shape->setTransform(offset);
      if (this->image_trace_dialog_->shouldDeleteImg()) {
        canvas_->document().execute(
                Commands::AddShape(canvas_->document().activeLayer(), new_shape),
                Commands::Select(&(canvas_->document()), {new_shape}),
                Commands::RemoveShape(selected_img_shape)
        );
      } else {
        canvas_->document().execute(
                Commands::AddShape(canvas_->document().activeLayer(), new_shape),
                Commands::Select(&(canvas_->document()), {new_shape})
        );
      }
    }
    this->image_trace_dialog_->reset();
  });

  connect(machine_manager_, &QDialog::accepted, this, &MainWindow::machineSettingsChanged);
  // Complex callbacks
  connect(welcome_dialog_, &WelcomeDialog::settingsChanged, [=]() {
    emit machineSettingsChanged();
  });
  connect(ui->actionExportGcode, &QAction::triggered, this, &MainWindow::exportGCodeFile);
  connect(ui->actionPreview, &QAction::triggered, [=]() {
    auto gen = canvas_->exportGcode();
    PreviewWindow *pw = new PreviewWindow(this,
                                          canvas_->document().width() / 10,
                                          canvas_->document().height() / 10);
    auto gen_gcode = make_shared<GCodeGenerator>(doc_panel_->currentMachine());
    ToolpathExporter exporter(gen_gcode.get());
    exporter.convertStack(canvas_->document().layers());
    gcode_player_->setGCode(QString::fromStdString(gen_gcode->toString()));
    pw->setPreviewPath(gen);
    pw->setRequiredTime(gcode_player_->requiredTime());
    pw->show();
  });
  connect(canvas_, &Canvas::cursorChanged, [=](Qt::CursorShape cursor) {
    if (cursor == Qt::ArrowCursor) {
      unsetCursor();
    } else {
      setCursor(cursor);
    }
  });
}

void MainWindow::closeEvent(QCloseEvent *event) {
  QSettings settings;
  settings.setValue("window/geometry", saveGeometry());
  settings.setValue("window/windowState", saveState());
  QMainWindow::closeEvent(event);
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

void MainWindow::showWelcomeDialog() {
  if (!MachineSettings().machines().empty()) return;
  QTimer::singleShot(0, [=]() {
    welcome_dialog_->show();
    welcome_dialog_->activateWindow();
    welcome_dialog_->raise();
  });
}