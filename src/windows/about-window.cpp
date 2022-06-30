#include<QDebug>
#include <QMessageBox>
#include <QSettings>
#include "about-window.h"
#include "ui_about-window.h"

AboutWindow::AboutWindow(QWidget *parent) :
     QDialog(parent),
     ui(new Ui::AboutWindow),
     BaseContainer() {
  ui->setupUi(this);
  initializeContainer();
//  ui->graphicsView
}

AboutWindow::~AboutWindow() {
  delete ui;
}
