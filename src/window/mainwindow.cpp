#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QListWidget>
#include <QQmlError>
#include <QQuickItem>
#include <QQuickWidget>
#include <QAbstractItemView>
#include <boost/range/adaptor/reversed.hpp>
#include <cmath>
#include <shape/bitmap_shape.h>
#include <widgets/spinbox_helper.h>
#include <widgets/canvas_text_edit.h>
#include <widgets/layer_widget.h>
#include <widgets/transform_widget.h>
#include <window/mainwindow.h>
#include <window/osxwindow.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    connect(ui->quickWidget, &QQuickWidget::statusChanged, this, &MainWindow::quickWidgetStatusChanged);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionClose, &QAction::triggered, this, &MainWindow::close);
    loadQML();
    loadQSS();
    updateMode();
    ui->objectParamDock->setWidget(new TransformWidget(ui->objectParamContents));
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
    ((SpinBoxHelper<QSpinBox> *)ui->spinBox)->lineEdit()->setStyleSheet("padding: 0 8px;");
    ((SpinBoxHelper<QSpinBox> *)ui->spinBox_2)->lineEdit()->setStyleSheet("padding: 0 8px;");
    ((SpinBoxHelper<QSpinBox> *)ui->spinBox_3)->lineEdit()->setStyleSheet("padding: 0 8px;");
    ((SpinBoxHelper<QSpinBox> *)ui->spinBox_4)->lineEdit()->setStyleSheet("padding: 0 8px;");
    ((SpinBoxHelper<QDoubleSpinBox> *)ui->doubleSpinBox_6)->lineEdit()->setStyleSheet("padding: 0 8px;");
    ((SpinBoxHelper<QDoubleSpinBox> *)ui->doubleSpinBox_7)->lineEdit()->setStyleSheet("padding: 0 8px;");
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

    canvas_ = ui->quickWidget->rootObject()->findChildren<VCanvas *>().first();
    connect(ui->actionCut, &QAction::triggered, canvas_, &VCanvas::editCut);
    connect(ui->actionCopy, &QAction::triggered, canvas_, &VCanvas::editCopy);
    connect(ui->actionPaste, &QAction::triggered, canvas_, &VCanvas::editPaste);
    connect(ui->actionUndo, &QAction::triggered, canvas_, &VCanvas::editUndo);
    connect(ui->actionRedo, &QAction::triggered, canvas_, &VCanvas::editRedo);
    connect(ui->actionSelect_All, &QAction::triggered, canvas_, &VCanvas::editSelectAll);
    connect(ui->actionGroup, &QAction::triggered, canvas_, &VCanvas::editGroup);
    connect(ui->actionUngroup, &QAction::triggered, canvas_, &VCanvas::editUngroup);
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
    connect(&canvas_->scene(), &Scene::layerChanged, this, &MainWindow::updateLayers);
    connect(&canvas_->scene(), &Scene::modeChanged, this, &MainWindow::updateMode);
    connect(&canvas_->scene(), &Scene::selectionsChanged, this, &MainWindow::updateSidePanel);
    connect(ui->fontComboBox, &QFontComboBox::currentFontChanged, [ = ](const QFont & font) {
        canvas_->setFont(font);
    });
    connect(ui->layerList->model(), &QAbstractItemModel::rowsMoved, this, &MainWindow::layerOrderChanged);
    connect(ui->layerList, &QListWidget::itemClicked, [ = ](QListWidgetItem * item) {
        canvas_->setActiveLayer(dynamic_cast<LayerWidget *>(ui->layerList->itemWidget(item))->layer_);
    });
    canvas_->scene().text_box_ = make_unique<CanvasTextEdit>(ui->inputFrame);
    canvas_->scene().text_box_->setGeometry(10, 10, 200, 200);
    canvas_->scene().text_box_->setStyleSheet("border:0");
    canvas_->fitWindow();
    updateLayers();
    updateMode();
    updateSidePanel();
}

void MainWindow::updateLayers() {
    ui->layerList->clear();

    for (auto &layer : boost::adaptors::reverse(canvas_->scene().layers())) {
        bool active = canvas_->scene().activeLayer().get() == layer.get();
        auto *list_widget = new LayerWidget(ui->layerList->parentWidget(), layer, active);
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
}

void MainWindow::layerOrderChanged(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                                   const QModelIndex &destinationParent, int destinationRow) {
    QList<LayerPtr> new_order;

    for (int i = 0; i < ui->layerList->count(); i++) {
        new_order << dynamic_cast<LayerWidget *>(ui->layerList->itemWidget(ui->layerList->item(i)))->layer_;
    }

    canvas_->setLayerOrder(new_order);
}

bool MainWindow::event(QEvent *e) {
    switch (e->type()) {
    case QEvent::CursorChange:
    case QEvent::UpdateRequest:
        break;

    case QEvent::NativeGesture:
        qInfo() << "Native Gesture!";
        canvas_->event(e);
        return true;

    case QEvent::KeyPress:
        // qInfo() << "Key event" << e;
        canvas_->event(e);
        return true;

    default:
        // qInfo() << "Event" << e;
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

void MainWindow::on_addLayer_clicked() {
    canvas_->scene().addLayer();
}

void MainWindow::updateMode() {
    ui->actionSelect->setChecked(false);
    ui->actionDrawRect->setChecked(false);
    ui->actionDrawLine->setChecked(false);
    ui->actionDrawOval->setChecked(false);
    ui->actionDrawPath->setChecked(false);
    ui->actionDrawText->setChecked(false);
    ui->actionDrawPolygon->setChecked(false);

    switch (canvas_->scene().mode()) {
    case Scene::Mode::SELECTING:
    case Scene::Mode::MULTI_SELECTING:
        ui->actionSelect->setChecked(true);
        break;

    case Scene::Mode::DRAWING_LINE:
        ui->actionDrawLine->setChecked(true);
        break;

    case Scene::Mode::DRAWING_RECT:
        ui->actionDrawRect->setChecked(true);
        break;

    case Scene::Mode::DRAWING_OVAL:
        ui->actionDrawOval->setChecked(true);
        break;

    case Scene::Mode::DRAWING_PATH:
        ui->actionDrawPath->setChecked(true);
        break;

    case Scene::Mode::DRAWING_TEXT:
        ui->actionDrawText->setChecked(true);
        break;

    default:
        break;
    }
}

void MainWindow::updateSidePanel() {
    QList<ShapePtr> &items = canvas_->scene().selections();
    /*if (items.length() > 1) {
        ui->tabWidget->setTabText(1, "Multiple Objects");
        ui->tabWidget->setTabEnabled(1, true);
        ui->tabWidget->setCurrentIndex(1);
    } else if (items.length() == 1) {
        ui->tabWidget->setTabText(1, "Object");
        ui->tabWidget->setTabEnabled(1, true);
        ui->tabWidget->setCurrentIndex(1);
    } else {
        ui->tabWidget->setTabText(1, "-");
        ui->tabWidget->setCurrentIndex(0);
        ui->tabWidget->setTabEnabled(1, false);
    }*/
    setOSXWindowTitleColor(this);
}
