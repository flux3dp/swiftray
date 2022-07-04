#include<QDebug>
#include <QMessageBox>
#include <QSettings>
#include "about-window.h"
#include "ui_about-window.h"
#include <QGraphicsItem>
#include <windows/osxwindow.h>

AboutWindow::AboutWindow(QWidget *parent) :
     QDialog(parent),
     ui(new Ui::AboutWindow),
     BaseContainer() {
  ui->setupUi(this);
  initializeContainer();
  ui->label_4->setPixmap(QPixmap(":/images/dark/icon-about.png"));
  // ui->label_4->setPixmap(QPixmap(isDarkMode() ? ":/images/dark/icon-about.png" : ":/images/icon-about.png"));
}

AboutWindow::~AboutWindow() {
  delete ui;
}
