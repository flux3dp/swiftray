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
    QUrl source("qrc:/main.qml");
    connect(ui->quickWidget, &QQuickWidget::statusChanged,
            this, &MainWindow::quickWidgetStatusChanged);
    connect(ui->quickWidget, &QQuickWidget::sceneGraphError,
            this, &MainWindow::sceneGraphError);
    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openFile);
    ui->quickWidget->setSource(source);
    ui->quickWidget->setResizeMode(QQuickWidget::SizeViewToRootObject);
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
    doc_ = ui->quickWidget->rootObject()->findChildren<VDoc *>().first();
}

void MainWindow::sceneGraphError(QQuickWindow::SceneGraphError, const QString &message) {
    //statusBar()->showMessage(message);
}


MainWindow::~MainWindow() {
    delete ui;
}
