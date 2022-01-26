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
#include <widgets/components/qdoublespinbox2.h>
#include <shape/bitmap-shape.h>
#include <widgets/components/canvas-text-edit.h>
#include <windows/mainwindow.h>
#include <windows/osxwindow.h>
#include <gcode/toolpath-exporter.h>
#include <gcode/generators/gcode-generator.h>
#include <gcode/generators/preview-generator.h>
#include <document-serializer.h>
#include <settings/file-path-settings.h>
#include <windows/preview-window.h>

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
       ui->toolBarConnection,
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
           (isDarkMode() ? ":/images/dark/icon-" : ":/images/icon-") + name
      ));
    }
  }
}

/**
 * @brief Check whether there is any unsaved change, and ask user to save
 * @return true if unsaved change is resolved
 *         false if unsaved change hasn't been resolved
 */
bool MainWindow::handleUnsavedChange() {
  if ( ! canvas_->document().currentFile().isEmpty()) { // current document is opened from a file
    if (canvas_->document().currentFileModified()) {    // some modifications have been performed on this document
      // TODO: Pop a dialog to ask whether save/cancel/don't save

    }
  }
  return true;
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
}

void MainWindow::onScalePlusClicked() {
  canvas_->setScaleWithCenter(qreal(qRound((canvas_->document().scale() + 0.05)*10))/10);
}

void MainWindow::onScaleMinusClicked() {
  canvas_->setScaleWithCenter(qreal(qRound((canvas_->document().scale() - 0.051)*10))/10);
}

void MainWindow::openFile() {
  if ( ! handleUnsavedChange()) {
    return;
  }
  QString default_open_dir = FilePathSettings::getDefaultFilePath();
  QString file_name = QFileDialog::getOpenFileName(this, "Open SVG", default_open_dir,
                                                   tr("SVG Files (*.svg);;BVG Files (*.bvg);;Scene Files (*.bb)"));

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
    } else {
      canvas_->loadSVG(data);
    }
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

  QFile file(canvas_->document().currentFile());
  if (file.open(QFile::ReadWrite)) {
    QDataStream stream(&file);
    canvas_->save(stream);
    file.close();
    qInfo() << "Saved";
  }
}

/**
 * @brief Save the document with a new filename
 */
void MainWindow::saveAsFile() {
  QString default_save_dir = FilePathSettings::getDefaultFilePath();

  QString filter = tr("Scene File (*.bb)");
  QString file_name = QFileDialog::getSaveFileName(this,
                                                   tr("Save Image"),
                                                   default_save_dir,
                                                   filter, &filter);

  //QString file_name = QFileDialog::getSaveFileName(this, "Save Image", ".", tr("Scene File (*.bb)"));
  QFile file(file_name);

  if (file.open(QFile::ReadWrite)) {
    // Update default file path
    QFileInfo file_info{file_name};
    FilePathSettings::setDefaultFilePath(file_info.absoluteDir().absolutePath());

    QDataStream stream(&file);
    canvas_->save(stream);
    file.close();
    canvas_->document().setCurrentFile(file_name);
    qInfo() << "Saved";
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
                                                   tr("Image Files (*.png *.jpg *.jpeg *.svg)"));

  if (!QFile::exists(file_name))
    return;

  QFileInfo file_info{file_name};
  FilePathSettings::setDefaultFilePath(file_info.absoluteDir().absolutePath());

  if (file_name.endsWith(".svg")) {
      QFile file(file_name);
    if (file.open(QFile::ReadOnly)) {
      QByteArray data = file.readAll();
      canvas_->loadSVG(data);
    }
  } else {
    QImage image;

    if (image.load(file_name)) {
      qInfo() << "File size:" << image.size();
      canvas_->importImage(image);
    }
  }

  canvas_->setMode(Canvas::Mode::Selecting);
}

void MainWindow::replaceImage() {
  QString default_open_dir = FilePathSettings::getDefaultFilePath();
  QString file_name = QFileDialog::getOpenFileName(this,
                                                   "Open Image",
                                                   default_open_dir,
                                                   tr("Image Files (*.png *.jpg)"));
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
  auto gen_gcode = make_shared<GCodeGenerator>(doc_panel_->currentMachine());
  ToolpathExporter exporter(gen_gcode.get());
  exporter.setDPMM(canvas_->document().settings().dpmm());
  exporter.setWorkAreaSize(QSizeF{canvas_->document().width() / 10, canvas_->document().height() / 10}); // TODO: Set machine work area in unit of mm
  exporter.convertStack(canvas_->document().layers());

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
  ui->actionUngroup->setEnabled(all_group);
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
#ifdef Q_OS_MACOS
  setOSXWindowTitleColor(this);
#endif
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
  machine_manager_ = new MachineManager(this);
  preferences_window_ = new PreferencesWindow(this);
  welcome_dialog_ = new WelcomeDialog(this);
  ui->joggingDock->setWidget(jogging_panel_);
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
  connect(canvas_, &Canvas::scaleChanged, this, &MainWindow::updateScale);
  connect(canvas_, &Canvas::canvasContextMenuOpened, this, &MainWindow::showCanvasPopMenu);
  // Monitor UI events
  connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newFile);
  connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
  connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveFile);
  connect(ui->actionSave_As, &QAction::triggered, this, &MainWindow::saveAsFile);
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
  connect(ui->actionMachineSettings, &QAction::triggered, machine_manager_, &MachineManager::show);
  connect(ui->actionPathOffset, &QAction::triggered, canvas_, &Canvas::genPathOffset);
  connect(ui->actionTrace, &QAction::triggered, canvas_, &Canvas::genImageTrace);
  connect(ui->actionInvert, &QAction::triggered, canvas_, &Canvas::invertImage);
  connect(ui->actionSharpen, &QAction::triggered, canvas_, &Canvas::sharpenImage);
  connect(ui->actionReplace_with, &QAction::triggered, this, &MainWindow::replaceImage);
  connect(ui->actionJogging, &QAction::triggered, this, &MainWindow::showJoggingPanel);
  connect(ui->actionCrop, &QAction::triggered, canvas_, &Canvas::cropImage);
  connect(ui->actionStart, &QAction::triggered, this, [=]() {
    generateGcode();
    gcode_player_->executeBtnClick();
  });
  connect(machine_manager_, &QDialog::accepted, this, &MainWindow::machineSettingsChanged);

  connect(ui->actionSaveClassics, &QAction::triggered, [=]() {
      QSettings settings(":/classicsUI.ini", QSettings::IniFormat);
      settings.setValue("window/windowState", saveState());
  });
  connect(ui->actionSaveEssential, &QAction::triggered, [=]() {
      QSettings settings(":/essentialUI.ini", QSettings::IniFormat);
      settings.setValue("window/windowState", saveState());
  });
  connect(ui->actionLoadClassics, &QAction::triggered, [=]() {
      QSettings settings(":/classicsUI.ini", QSettings::IniFormat);
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
  ui->quickWidget->connect(pasteAction_, &QAction::triggered, canvas_, &Canvas::editPaste);
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
  ui->toolBarConnection->addWidget(portComboBox_);
  ui->toolBarConnection->addWidget(baudComboBox_);
  QTimer *timer = new QTimer(this);
  QList<QSerialPortInfo> portList;
  baudComboBox_->addItem("9600");
  baudComboBox_->addItem("12800");
  baudComboBox_->addItem("25600");
  baudComboBox_->addItem("51200");
  baudComboBox_->addItem("57600");
  baudComboBox_->addItem("102400");
  baudComboBox_->addItem("115200");
  baudComboBox_->addItem("204800");
  connect(timer, &QTimer::timeout, [=]() {
    const auto infos = QSerialPortInfo::availablePorts();
    int current_index = portComboBox_->currentIndex() > -1 ? portComboBox_->currentIndex() : 0;
    portComboBox_->clear();
    for (const QSerialPortInfo &info : infos) {
      portComboBox_->addItem(info.portName());
    }
    portComboBox_->setCurrentIndex(current_index > portComboBox_->count() - 1 ? portComboBox_->count() - 1 : current_index);
  });
  connect(ui->actionConnect, &QAction::triggered, [=]() {
    if (SerialPort::getInstance().isConnected()) {
      qInfo() << "[SerialPort] Disconnect";
      SerialPort::getInstance().stop(); // disconnect
      return;
    }
    QString port = portComboBox_->currentText();
    QString baudrate = baudComboBox_->currentText();
    QString full_port_path;
    if (port.startsWith("tty")) { // Linux/macOSX
      full_port_path += "/dev/";
      full_port_path += port;
    } else { // Windows COMx
      full_port_path = port;
    }
    qInfo() << "[SerialPort] Connecting" << port << baudrate;
    bool rv = SerialPort::getInstance().start(full_port_path.toStdString().c_str(), baudrate.toInt());
    if (rv == false) {
      // Do something?
      return;
    }
  });
  timer->start(5000);
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
  boldToolButton->setIcon(QIcon(isDarkMode() ? ":/images/dark/icon-bold.png" : ":/images/icon-bold.png"));
  italicToolButton->setIcon(QIcon(isDarkMode() ? ":/images/dark/icon-I.png" : ":/images/icon-I.png"));
  underlineToolButton->setIcon(QIcon(isDarkMode() ? ":/images/dark/icon-U.png" : ":/images/icon-U.png"));
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
  doubleSpinBoxLetterSpacing->setSingleStep(0.1);
  doubleSpinBoxLineHeight->setDecimals(1);
  doubleSpinBoxLineHeight->setMaximum(100);
  doubleSpinBoxLineHeight->setSingleStep(0.1);
  spinBoxSize->setMaximum(1000);

  QFont initialFont = QFont("Tahoma", 100, QFont::Bold);

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
    font_panel_->setFont(font, font_panel_->lineHeight());
  });

  connect(spinBoxSize, spin_int_event, [=](int value) {
    QFont font = font_panel_->font();
    font.setPointSize(int(value));
    font_panel_->setFont(font, font_panel_->lineHeight());
  }); 

  connect(doubleSpinBoxLetterSpacing, spin_event, [=](double value) {
    QFont font = font_panel_->font();
    font.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, value);
    font_panel_->setFont(font, font_panel_->lineHeight());
  });

  connect(doubleSpinBoxLineHeight, spin_event, canvas(), &Canvas::setLineHeight);

  connect(doubleSpinBoxLineHeight, spin_event, font_panel_, &FontPanel::setLineHeight);

  connect(boldToolButton, &QToolButton::toggled, [=](bool checked) {
    QFont font = font_panel_->font();
    font.setBold(checked);
    font_panel_->setFont(font, font_panel_->lineHeight());
  });

  connect(italicToolButton, &QToolButton::toggled, [=](bool checked) {
    QFont font = font_panel_->font();
    font.setItalic(checked);
    font_panel_->setFont(font, font_panel_->lineHeight());
  });

  connect(underlineToolButton, &QToolButton::toggled, [=](bool checked) {
    QFont font = font_panel_->font();
    font.setUnderline(checked);
    font_panel_->setFont(font, font_panel_->lineHeight());
  });

  connect(font_panel_, &FontPanel::fontSettingChanged, [=]() {
    QFont font = font_panel_->font();
    fontComboBox->setCurrentFont(font);
    doubleSpinBoxLetterSpacing->setValue(font.letterSpacing());
    spinBoxSize->setValue(font.pointSize());
    boldToolButton->setChecked(font.bold());
    italicToolButton->setChecked(font.italic());
    underlineToolButton->setChecked(font.underline());
  });

  connect(font_panel_, &FontPanel::lineHeightChanged, [=](double line_height) {
    doubleSpinBoxLineHeight->setValue(line_height);
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
  labelX->setText("X");
  labelY->setText("Y");
  labelRotation->setText(tr("Rotation"));
  labelWidth->setText(tr("Width"));
  labelHeight->setText(tr("Height"));
  doubleSpinBoxX->setMaximum(9999);
  doubleSpinBoxY->setMaximum(9999);
  doubleSpinBoxRotation->setMaximum(9999);
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
  ui->toolBarTransform->addWidget(labelRotation);
  ui->toolBarTransform->addWidget(doubleSpinBoxRotation);
  ui->toolBarTransform->addWidget(labelWidth);
  ui->toolBarTransform->addWidget(doubleSpinBoxWidth);
  ui->toolBarTransform->addWidget(labelHeight);
  ui->toolBarTransform->addWidget(doubleSpinBoxHeight);

  auto spin_event = QOverload<double>::of(&QDoubleSpinBox::valueChanged);

  connect(canvas(), &Canvas::transformChanged, [=](qreal x, qreal y, qreal r, qreal w, qreal h) {
    x_ = x/10;
    y_ = y/10;
    r_ = r;
    w_ = w/10;
    h_ = h/10;
    doubleSpinBoxX->setValue(x/10);
    doubleSpinBoxY->setValue(y/10);
    doubleSpinBoxRotation->setValue(r);
    doubleSpinBoxWidth->setValue(w/10);
    doubleSpinBoxHeight->setValue(h/10);
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
    w_ = doubleSpinBoxWidth->value();
    updateToolbarTransform();
  });

  connect(doubleSpinBoxHeight, spin_event, [=]() {
    h_ = doubleSpinBoxHeight->value();
    updateToolbarTransform();
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
  QToolButton *minusBtn = new QToolButton(ui->quickWidget);
  QToolButton *plusBtn = new QToolButton(ui->quickWidget);
  scale_block_->setGeometry(ui->quickWidget->geometry().left() + 60, this->size().height() - 110, 50, 30);
  scale_block_->setStyleSheet("QPushButton { border: none; } QPushButton::hover { border: none; background-color: transparent }");
  minusBtn->setIcon(QIcon(isDarkMode() ? ":/images/dark/icon-plus.png" : ":/images/icon-plus.png"));
  minusBtn->setGeometry(ui->quickWidget->geometry().left() + 30, this->size().height() - 110, 40, 30);
  minusBtn->setStyleSheet("QToolButton { border: none; } QToolButton::hover { border: none; background-color: transparent }");
  plusBtn->setIcon(QIcon(isDarkMode() ? ":/images/dark/icon-plus.png" : ":/images/icon-plus.png"));
  plusBtn->setGeometry(ui->quickWidget->geometry().left() + 100, this->size().height() - 110, 40, 30);
  plusBtn->setStyleSheet("QToolButton { border: none; } QToolButton::hover { border: none; background-color: transparent }");

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

    scale_block_->setGeometry(ui->quickWidget->geometry().left() + 60, this->size().height() - 110, 50, 30);
    minusBtn->setGeometry(ui->quickWidget->geometry().left() + 30, this->size().height() - 110, 40, 30);
    plusBtn->setGeometry(ui->quickWidget->geometry().left() + 100, this->size().height() - 110, 40, 30);
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

  connect(minusBtn, &QAbstractButton::clicked, this, &MainWindow::onScaleMinusClicked);
  connect(plusBtn, &QAbstractButton::clicked, this, &MainWindow::onScalePlusClicked);
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
  if (!MachineSettings().machines().empty()) return;
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

void MainWindow::generateGcode() {
  auto gen_gcode = make_shared<GCodeGenerator>(doc_panel_->currentMachine());
  ToolpathExporter exporter(gen_gcode.get());
  exporter.setDPMM(canvas_->document().settings().dpmm());
  exporter.setWorkAreaSize(QSizeF{canvas_->document().width() / 10, canvas_->document().height() / 10}); // TODO: Set machine work area in unit of mm
  exporter.convertStack(canvas_->document().layers());
  gcode_player_->setGCode(QString::fromStdString(gen_gcode->toString()));
}

void MainWindow::genPreviewWindow() {
  auto gen = make_shared<PreviewGenerator>(doc_panel_->currentMachine());
  ToolpathExporter preview_exporter(gen.get());
  preview_exporter.convertStack(canvas_->document().layers());
  PreviewWindow *pw = new PreviewWindow(this,
                                        canvas_->document().width() / 10,
                                        canvas_->document().height() / 10);
  auto gen_gcode = make_shared<GCodeGenerator>(doc_panel_->currentMachine());
  ToolpathExporter gcode_exporter(gen_gcode.get());
  gcode_exporter.convertStack(canvas_->document().layers());
  gcode_player_->setGCode(QString::fromStdString(gen_gcode->toString()));
  pw->setPreviewPath(gen);
  pw->setRequiredTime(gcode_player_->requiredTime());
  pw->show();
}
