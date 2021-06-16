#include <QDebug>
#include <QFileDialog>
#include <QListWidget>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickWidget>
#include <QAbstractItemView>
#include <boost/range/adaptor/reversed.hpp>
#include <cmath>
#include <shape/bitmap-shape.h>
#include <widgets/spinbox-helper.h>
#include <widgets/canvas-text-edit.h>
#include <window/mainwindow.h>
#include <window/osxwindow.h>
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  connect(ui->quickWidget, &QQuickWidget::statusChanged, this, &MainWindow::quickWidgetStatusChanged);
  connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
  connect(ui->actionClose, &QAction::triggered, this, &MainWindow::close);
  loadQML();
  loadQSS();
  loadWidgets();
  updateMode();
}

void MainWindow::loadQML() {
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
  // QPaintedItem in Qt6 does not support Metal rendering yet, so it will be slow using metal RHI
  qInfo() << "Falling back to OpenGLRhi in QT6";
  ((QQuickWindow *)ui->quickWidget)->setGraphicsApi(QSGRendererInterface::OpenGLRhi);
  connect(ui->quickWidget, &QQuickWidget::sceneGraphError, this, &MainWindow::sceneGraphError);
#endif
  QUrl source("qrc:/src/window/main.qml");
  ui->quickWidget->setResizeMode(QQuickWidget::ResizeMode::SizeRootObjectToView);
  ui->quickWidget->setSource(source);
  ui->quickWidget->show();
}

void MainWindow::loadQSS() {
  QFile file(":/styles/vecty.qss");
  file.open(QFile::ReadOnly);
  QString styleSheet = QLatin1String(file.readAll());
  setStyleSheet(styleSheet);
  ((SpinBoxHelper<QSpinBox> *) ui->spinBox_4)->lineEdit()->setStyleSheet("padding: 0 8px;");
  ((SpinBoxHelper<QDoubleSpinBox> *) ui->doubleSpinBox_6)->lineEdit()->setStyleSheet("padding: 0 8px;");
  ((SpinBoxHelper<QDoubleSpinBox> *) ui->doubleSpinBox_7)->lineEdit()->setStyleSheet("padding: 0 8px;");
}

void MainWindow::openFile() {
  QString file_name = QFileDialog::getOpenFileName(this, "Open SVG", ".", tr("SVG Files (*.svg)"));

  if (!QFile::exists(file_name))
    return;

  QFile file(file_name);

  if (file.open(QFile::ReadOnly)) {
    QByteArray data = file.readAll();
    qInfo() << "File size:" << data.size();
    canvas_->loadSVG(data);
  }
}

void MainWindow::openImageFile() {
  QString file_name = QFileDialog::getOpenFileName(this, "Open Image", ".", tr("Image Files (*.png *.jpg)"));

  if (!QFile::exists(file_name))
    return;

  QImage image;

  if (image.load(file_name)) {
    qInfo() << "File size:" << image.size();
    canvas_->importImage(image);
  }
}

void MainWindow::quickWidgetStatusChanged(QQuickWidget::Status status) {
  if (status == QQuickWidget::Error) {
    const auto widgetErrors = this->ui->quickWidget->errors();

    for (const QQmlError &error : widgetErrors)
      qInfo() << error.toString();

    Q_ASSERT_X(false, "QQuickWidget Initialization", "QQuickWidget failed to initialize");
  }

  // Set the owner of vcanvas
  canvas_ = ui->quickWidget->rootObject()->findChildren<VCanvas *>().first();
  doc_ = &canvas_->document();
  connect(ui->actionCut, &QAction::triggered, canvas_, &VCanvas::editCut);
  connect(ui->actionCopy, &QAction::triggered, canvas_, &VCanvas::editCopy);
  connect(ui->actionPaste, &QAction::triggered, canvas_, &VCanvas::editPaste);
  connect(ui->actionUndo, &QAction::triggered, canvas_, &VCanvas::editUndo);
  connect(ui->actionRedo, &QAction::triggered, canvas_, &VCanvas::editRedo);
  connect(ui->actionSelect_All, &QAction::triggered, canvas_, &VCanvas::editSelectAll);
  connect(ui->actionGroup, &QAction::triggered, canvas_, &VCanvas::editGroup);
  connect(ui->actionUngroup, &QAction::triggered, canvas_, &VCanvas::editUngroup);
  connect(ui->actionExportGcode, &QAction::triggered, canvas_, &VCanvas::exportGcode);
  connect(ui->actionDrawRect, &QAction::triggered, canvas_, &VCanvas::editDrawRect);
  connect(ui->actionDrawOval, &QAction::triggered, canvas_, &VCanvas::editDrawOval);
  connect(ui->actionDrawLine, &QAction::triggered, canvas_, &VCanvas::editDrawLine);
  connect(ui->actionDrawPath, &QAction::triggered, canvas_, &VCanvas::editDrawPath);
  connect(ui->actionDrawText, &QAction::triggered, canvas_, &VCanvas::editDrawText);
  connect(ui->actionDrawPhoto, &QAction::triggered, this, &MainWindow::openImageFile);
  connect(ui->actionUnionBtn, &QAction::triggered, canvas_, &VCanvas::editUnion);
  connect(ui->actionSubtractBtn, &QAction::triggered, canvas_, &VCanvas::editSubtract);
  connect(ui->actionIntersectBtn, &QAction::triggered, canvas_, &VCanvas::editIntersect);
  connect(ui->actionDiffBtn, &QAction::triggered, canvas_, &VCanvas::editDifference);
  connect(ui->actionGroupBtn, &QAction::triggered, canvas_, &VCanvas::editGroup);
  connect(ui->actionUngroupBtn, &QAction::triggered, canvas_, &VCanvas::editUngroup);
  connect(doc_, &Document::layerChanged, this, &MainWindow::updateLayers);
  connect(doc_, &Document::modeChanged, this, &MainWindow::updateMode);
  connect(doc_, &Document::selectionsChanged, this, &MainWindow::updateSidePanel);
  connect(ui->fontComboBox, &QFontComboBox::currentFontChanged, [=](const QFont &font) {
    canvas_->setFont(font);
  });
  connect(ui->layerList->model(), &QAbstractItemModel::rowsMoved, this, &MainWindow::layerOrderChanged);
  connect(ui->layerList, &QListWidget::itemClicked, [=](QListWidgetItem *item) {
    canvas_->setActiveLayer(dynamic_cast<LayerListItem *>(ui->layerList->itemWidget(item))->layer_);
  });
  doc_->text_box_ = make_unique<CanvasTextEdit>(ui->inputFrame);
  doc_->text_box_->setGeometry(10, 10, 200, 200);
  doc_->text_box_->setStyleSheet("border:0");
  canvas_->fitWindow();
  updateLayers();
  updateMode();
  updateSidePanel();
  canvas_->setScreenSize(ui->quickWidget->geometry().size());
  canvas_->setScreenOffset(ui->quickWidget->parentWidget()->mapToParent(ui->quickWidget->geometry().topLeft()));
}

void MainWindow::updateLayers() {
  ui->layerList->clear();

  for (auto &layer : boost::adaptors::reverse(doc_->layers())) {
    bool active = doc_->activeLayer().get() == layer.get();
    auto *list_widget = new LayerListItem(ui->layerList->parentWidget(), layer, active);
    auto *list_item = new QListWidgetItem(ui->layerList);
    auto size = list_widget->size();
    list_item->setSizeHint(size);
    ui->layerList->setItemWidget(list_item, list_widget);

    if (active) {
      ui->layerList->setCurrentItem(list_item);
    }
  }

  if (ui->layerList->currentItem()) {
    ui->layerList->scrollToItem(ui->layerList->currentItem(), QAbstractItemView::PositionAtCenter);
  }
  if (layer_params_panel_ != nullptr) {
    layer_params_panel_->updateLayer(doc_->activeLayer());
  }
}

void MainWindow::layerOrderChanged(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                                   const QModelIndex &destinationParent, int destinationRow) {
  QList<LayerPtr> new_order;

  for (int i = ui->layerList->count() - 1; i >= 0; i--) {
    new_order << dynamic_cast<LayerListItem *>(ui->layerList->itemWidget(ui->layerList->item(i)))->layer_;
  }

  canvas_->setLayerOrder(new_order);
}

bool MainWindow::event(QEvent *e) {
  QNativeGestureEvent *nge0;
  QNativeGestureEvent nge1(Qt::NativeGestureType::ZoomNativeGesture, QPointF(), QPointF(), QPointF(), 0, 0, 0);
  switch (e->type()) {
    case QEvent::CursorChange:
    case QEvent::UpdateRequest:
      break;

    case QEvent::NativeGesture:
    case QEvent::KeyPress:
      canvas_->event(e);
      return true;

    default:
      // qInfo() << "Event" << e;
      break;
  }

  return QMainWindow::event(e);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
  canvas_->setScreenSize(ui->quickWidget->geometry().size());
  canvas_->setScreenOffset(ui->quickWidget->parentWidget()->mapToParent(ui->quickWidget->geometry().topLeft()));
}

void MainWindow::sceneGraphError(QQuickWindow::SceneGraphError, const QString &message) {
  // statusBar()->showMessage(message);
}

MainWindow::~MainWindow() {
  delete ui;
}

void MainWindow::updateMode() {
  ui->actionSelect->setChecked(false);
  ui->actionDrawRect->setChecked(false);
  ui->actionDrawLine->setChecked(false);
  ui->actionDrawOval->setChecked(false);
  ui->actionDrawPath->setChecked(false);
  ui->actionDrawText->setChecked(false);
  ui->actionDrawPolygon->setChecked(false);

  switch (doc_->mode()) {
    case Document::Mode::Selecting:
    case Document::Mode::MultiSelecting:
      ui->actionSelect->setChecked(true);
      break;

    case Document::Mode::LineDrawing:
      ui->actionDrawLine->setChecked(true);
      break;

    case Document::Mode::RectDrawing:
      ui->actionDrawRect->setChecked(true);
      break;

    case Document::Mode::OvalDrawing:
      ui->actionDrawOval->setChecked(true);
      break;

    case Document::Mode::PathDrawing:
      ui->actionDrawPath->setChecked(true);
      break;

    case Document::Mode::TextDrawing:
      ui->actionDrawText->setChecked(true);
      break;

    default:
      break;
  }
}

void MainWindow::updateSidePanel() {
  QList<ShapePtr> &items = doc_->selections();
  setOSXWindowTitleColor(this);
}


void MainWindow::loadWidgets() {
  transform_panel_ = make_unique<TransformPanel>(ui->objectParamDock);
  layer_params_panel_ = make_unique<LayerParamsPanel>(ui->layerDockContents);
  ui->objectParamDock->setWidget(transform_panel_.get());
  ui->layerDockContents->layout()->addWidget(layer_params_panel_.get());
  if (canvas_ != nullptr && doc_->layers().size() > 0) {
    layer_params_panel_->updateLayer(doc_->activeLayer());
  }
  add_layer_btn_ = make_unique<QToolButton>(ui->layerList);
  add_layer_btn_->setIcon(QIcon(":/images/icon-plus-01.png"));
  QRect geometry = QRect(215, 190, 35, 35);
  qInfo() << "GEO" << geometry;
  add_layer_btn_->setGeometry(geometry);
  add_layer_btn_->setIconSize(QSize(24, 24));
  add_layer_btn_->raise();
  add_layer_btn_->show();
  connect(add_layer_btn_.get(), &QAbstractButton::clicked, [=]() {
    canvas_->addEmptyLayer();
  });
  transform_panel_->setTransformControl(&canvas_->transformControl());
}