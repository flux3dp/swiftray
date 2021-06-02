#include <QQuickWidget>
#include <QQmlError>
#include <QQuickItem>
#include <QFileDialog>
#include <QListWidget>
#include <QDebug>
#include <window/mainwindow.h>
#include <widgets/layer_widget.h>
#include "ui_mainwindow.h"
#include <window/osxwindow.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {
    ui->setupUi(this);
#if QT_VERSION > QT_VERSION_CHECK(6, 0, 0)
    //QPaintedItem in Qt6 does not support Metal rendering yet
    ((QQuickWindow *)ui->quickWidget)->setGraphicsApi(QSGRendererInterface::OpenGLRhi);
    connect(ui->quickWidget, &QQuickWidget::sceneGraphError,
            this, &MainWindow::sceneGraphError);
#endif
    connect(ui->quickWidget, &QQuickWidget::statusChanged,
            this, &MainWindow::quickWidgetStatusChanged);
    QUrl source("qrc:/src/window/main.qml");
    ui->quickWidget->setSource(source);
    ui->quickWidget->show();
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionClose, &QAction::triggered, this, &MainWindow::close);
    this->setStyleSheet(
        "QToolBar { background-color: #F0F0F0; spacing: 8px; border-right: 1px solid #CCC; }"
        "#tabWidget { border: 0; }"
        "#layerTab { border: 0; border-left: 1px solid #CCC;  }"
        "#objectTab { border: 0; border-left: 1px solid #CCC; }"
        "#scrollAreaWidgetContent { background: #F8F8F8 }"
        "#objectFrame1 { border: 0; background: #F8F8F8;}"
        "#objectFrame2 { border: 0; }"
        "#objectFrame3 { border: 0; }"
        "#objectFrame4 { border: 0; }"
        "#parameterLabelFrame { border: 0; border-top: 1px solid #CCC; border-bottom: 1px solid #CCC; background: #F0F0F0; }"
        "#parameterFrame { border: 0; }"
        "#addLayerFrame { border: 0; background: white; }"
    );
    ui->layerList->setStyleSheet(
        "border: 0"
    );
    ui->toolBar->setStyleSheet(
        "QToolButton{ border-width:0px; border-radius: 0px; }"
        "QToolButton:hover { background:#CCC; }"
        "QToolButton:pressed { background:#BBB; }"
        "QToolButton:focused { background:#555; }"
        "QToolButton:checked { background:#BBB; }"
    );
    updateMode();
}

void MainWindow::openFile() {
    QString file_name = QFileDialog::getOpenFileName(this);

    if (QFile::exists(file_name)) {
        QMimeType mime = QMimeDatabase().mimeTypeForFile(file_name);
        QFile file(file_name);

        if (file.open(QFile::ReadOnly)) {
            QByteArray data = file.readAll();
            qInfo() << "File size:" << data.size();
            canvas_->loadSVG(data);
        }
    }
}

void MainWindow::quickWidgetStatusChanged(QQuickWidget::Status status) {
    qInfo() << status;

    if (status == QQuickWidget::Error) {
        QStringList errors;
        const auto widgetErrors = this->ui->quickWidget->errors();

        for (const QQmlError &error : widgetErrors)
            errors.append(error.toString());

        //statusBar()->showMessage(errors.join(QStringLiteral(", ")));
    } else {
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
    connect(ui->btnUnion, SIGNAL(clicked()), canvas_, SLOT(editUnion()));
    connect(ui->btnSubtract, SIGNAL(clicked()), canvas_, SLOT(editSubtract()));
    connect(ui->btnIntersect, SIGNAL(clicked()), canvas_, SLOT(editIntersect()));
    connect(ui->btnDiff, SIGNAL(clicked()), canvas_, SLOT(editDifference()));
    connect((QObject *)&canvas_->scene(), SIGNAL(layerChanged()), this, SLOT(updateLayers()));
    connect((QObject *)&canvas_->scene(), SIGNAL(modeChanged()), this, SLOT(updateMode()));
    connect((QObject *)&canvas_->scene(), SIGNAL(selectionsChanged()), this, SLOT(updateSidePanel()));
    ui->layerList->setDragDropMode(QAbstractItemView::InternalMove);
    updateLayers();
    updateMode();
    updateSidePanel();
}

void MainWindow::updateLayers() {
    ui->layerList->clear();

    for (Layer &layer : canvas_->scene().layers()) {
        LayerWidget *list_widget = new LayerWidget(ui->layerList->parentWidget(), layer);
        QListWidgetItem *list_item = new QListWidgetItem(ui->layerList);
        QSize size = list_widget->size();
        list_item->setSizeHint(size);
        ui->layerList->setItemWidget(list_item, list_widget);
    }
}


bool MainWindow::event(QEvent *e)  {
    switch (e->type()) {
    case QEvent::NativeGesture:
        qInfo() << "Native Gesture!";
        canvas_->event(e);
        return true;

    case QEvent::KeyPress:
        //qInfo() << "Key event" << e;
        canvas_->event(e);
        return true;

    default:
        break;
    }

    return QMainWindow::event(e);
}

void MainWindow::sceneGraphError(QQuickWindow::SceneGraphError, const QString &message) {
    //statusBar()->showMessage(message);
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
    }
}

void MainWindow::updateSidePanel() {
    qInfo() << "Update side panel";
    QList<ShapePtr> &items = canvas_->scene().selections();

    if (items.length() > 1) {
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
    }

    setOSXWindowTitleColor(this);
}
