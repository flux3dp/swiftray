#include<QDebug>
#include <QMessageBox>
#include <QSettings>
#include "about-window.h"
#include "ui_about-window.h"
#include <QGraphicsItem>
#include <windows/osxwindow.h>
#include "config.h"

AboutWindow::AboutWindow(QWidget *parent) :
     QDialog(parent),
     ui(new Ui::AboutWindow),
     BaseContainer() {
  ui->setupUi(this);
  initializeContainer();

  QString ver_str{"<html><head/><body><p>"};
  ver_str.append(tr("Version ")); 
  ver_str.append(QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_BUILD)); 
  ver_str.append(QString{VERSION_SUFFIX});
  ver_str.append("</p></body></html>");
  qInfo() << "App version: " << ver_str;
  ui->labelVersion->setText(ver_str);

  ui->label_4->setPixmap(QPixmap(isDarkMode() ? ":/resources/images/dark/icon-about.png" : ":/resources/images/icon-about.png"));
}

AboutWindow::~AboutWindow() {
  delete ui;
}
