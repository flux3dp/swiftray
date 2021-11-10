#include <QDebug>
#include <QFileDialog>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickWidget>
#include <shape/bitmap-shape.h>
#include <widgets/components/canvas-text-edit.h>
#include <windows/mainwindow.h>
#include <windows/osxwindow.h>
#include <gcode/toolpath-exporter.h>
#include <gcode/generators/gcode-generator.h>
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
  updateSelections();
  showWelcomeDialog();
  setScaleBlock();
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

  for (auto &shape : canvas_->document().selections()) {
    if (shape->type() != Shape::Type::Group) all_group = false;

    if (shape->type() != Shape::Type::Path && shape->type() != Shape::Type::Text) all_path = false;
    if (shape->type() != Shape::Type::Path) all_geometry = false;
    if (shape->type() != Shape::Type::Bitmap) all_image = false;
  }

  cutAction_->setEnabled(items.size() > 0);
  copyAction_->setEnabled(items.size() > 0);
  //pasteAction_;
  duplicateAction_->setEnabled(items.size() > 0);
  deleteAction_->setEnabled(items.size() > 0);
  groupAction_->setEnabled(items.size() > 1);
  ungroupAction_->setEnabled(all_group);

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
  ui->actionInvert->setEnabled(items.size() == 1 && all_image);
  ui->actionReplace_with->setEnabled(items.size() == 1 && all_image);
  ui->actionPathOffset->setEnabled(all_geometry);
  ui->actionSharpen->setEnabled(items.size() == 1 && all_image);
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
  connect(machine_manager_, &QDialog::accepted, this, &MainWindow::machineSettingsChanged);
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
  popMenu_ = new QMenu(this);
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

  addAction(cutAction_);
  addAction(copyAction_);
  addAction(pasteAction_);
  addAction(pasteInPlaceAction_);
  addAction(duplicateAction_);
  addAction(deleteAction_);
  addAction(groupAction_);
  addAction(ungroupAction_);

  connect(cutAction_, &QAction::triggered, canvas_, &Canvas::editCut);
  connect(copyAction_, &QAction::triggered, canvas_, &Canvas::editCopy);
  connect(pasteAction_, &QAction::triggered, canvas_, &Canvas::editPaste);
  connect(pasteInPlaceAction_, &QAction::triggered, canvas_, &Canvas::editPasteInPlace);
  connect(duplicateAction_, &QAction::triggered, canvas_, &Canvas::editDuplicate);
  connect(deleteAction_, &QAction::triggered, canvas_, &Canvas::editCut);
  connect(groupAction_, &QAction::triggered, canvas_, &Canvas::editGroup);
  connect(ungroupAction_, &QAction::triggered, canvas_, &Canvas::editUngroup);

  setContextMenuPolicy(Qt::CustomContextMenu);
}

void MainWindow::setScaleBlock() {
  scale_block_ = new QPushButton("100%", ui->quickWidget);
  QToolButton *minusBtn = new QToolButton(ui->quickWidget);
  QToolButton *plusBtn = new QToolButton(ui->quickWidget);
  scale_block_->setGeometry(ui->quickWidget->geometry().left() + 60, this->size().height() - 100, 50, 30);
  scale_block_->setStyleSheet("QPushButton { border: none; } QPushButton::hover { border: none; background-color: transparent }");
  minusBtn->setIcon(QIcon(":/images/icon-plus.png"));
  minusBtn->setGeometry(ui->quickWidget->geometry().left() + 30, this->size().height() - 100, 40, 30);
  minusBtn->setStyleSheet("QToolButton { border: none; } QToolButton::hover { border: none; background-color: transparent }");
  plusBtn->setIcon(QIcon(":/images/icon-plus.png"));
  plusBtn->setGeometry(ui->quickWidget->geometry().left() + 100, this->size().height() - 100, 40, 30);
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

void MainWindow::genPreviewWindow() {
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
}
