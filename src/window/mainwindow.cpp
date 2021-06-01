#include <QQuickWidget>
#include <QQmlError>
#include <QQuickItem>
#include <QFileDialog>
#include <QListWidget>
#include <QDebug>
#include <window/mainwindow.h>
#include "ui_mainwindow.h"
#include <widgets/layer_widget.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {
    ui->setupUi(this);
    //QPaintedItem does not support Metal rendering yet
    ((QQuickWindow *)ui->quickWidget)->setGraphicsApi(QSGRendererInterface::OpenGLRhi);
    connect(ui->quickWidget, &QQuickWidget::statusChanged,
            this, &MainWindow::quickWidgetStatusChanged);
    connect(ui->quickWidget, &QQuickWidget::sceneGraphError,
            this, &MainWindow::sceneGraphError);
    QUrl source("qrc:/src/window/main.qml");
    ui->quickWidget->setSource(source);
    ui->quickWidget->show();
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    connect(ui->actionClose, &QAction::triggered, this, &MainWindow::close);
}

void MainWindow::openFile() {
    qInfo() << "Open file";
    QString file_name = QFileDialog::getOpenFileName(this);
    qInfo() << "Loading" << file_name;

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

    qInfo() << "Children " << ui->quickWidget->rootObject()->children();
    canvas_ = ui->quickWidget->rootObject()->findChildren<VCanvas *>().first();
    qInfo() << canvas_;
    connect(ui->actionCut, &QAction::triggered, canvas_, &VCanvas::editCut);
    connect(ui->actionCopy, &QAction::triggered, canvas_, &VCanvas::editCopy);
    connect(ui->actionPaste, &QAction::triggered, canvas_, &VCanvas::editPaste);
    connect(ui->actionUndo, &QAction::triggered, canvas_, &VCanvas::editUndo);
    connect(ui->actionRedo, &QAction::triggered, canvas_, &VCanvas::editRedo);
    connect(ui->actionSelect_All, &QAction::triggered, canvas_, &VCanvas::editSelectAll);
    connect(ui->actionGroup, &QAction::triggered, canvas_, &VCanvas::editGroup);
    connect(ui->actionUngroup, &QAction::triggered, canvas_, &VCanvas::editUngroup);
    connect((QObject *)canvas_->scenePtr(), SIGNAL(layerChanged()), this, SLOT(updateLayers()));
    ui->layerList->setDragDropMode(QAbstractItemView::InternalMove);
    // Enable drag & drop ordering of items.
    updateLayers();
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
