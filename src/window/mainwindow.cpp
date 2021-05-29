#include <QQuickWidget>
#include <QQmlError>
#include <QQuickItem>
#include <QFileDialog>
#include <QDebug>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow) {
    ui->setupUi(this);
    //((QQuickWindow *)ui->quickWidget)->setGraphicsApi(QSGRendererInterface::OpenGLRhi);
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
    qInfo() << doc_ << "loading" << file_name;
    doc_->load(file_name);
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
    doc_ = new VDoc(ui->quickWidget->rootObject());
    doc_->setCanvas(canvas_);
    qInfo() << doc_;
    connect(ui->actionCut, &QAction::triggered, canvas_, &VCanvas::editCut);
    connect(ui->actionCopy, &QAction::triggered, canvas_, &VCanvas::editCopy);
    connect(ui->actionPaste, &QAction::triggered, canvas_, &VCanvas::editPaste);
    connect(ui->actionUndo, &QAction::triggered, canvas_, &VCanvas::editUndo);
    connect(ui->actionRedo, &QAction::triggered, canvas_, &VCanvas::editRedo);
    connect(ui->actionSelect_All, &QAction::triggered, canvas_, &VCanvas::editSelectAll);
    connect(ui->actionGroup, &QAction::triggered, canvas_, &VCanvas::editGroup);
    connect(ui->actionUngroup, &QAction::triggered, canvas_, &VCanvas::editUngroup);
}

void MainWindow::sceneGraphError(QQuickWindow::SceneGraphError, const QString &message) {
    //statusBar()->showMessage(message);
}


MainWindow::~MainWindow() {
    delete ui;
}
