#include<QDebug>
#include <QMessageBox>
#include <QSettings>
#include "about-window.h"
#include "ui_about-window.h"
#include <QGraphicsItem>

AboutWindow::AboutWindow(QWidget *parent) :
     QDialog(parent),
     ui(new Ui::AboutWindow),
     BaseContainer() {
  ui->setupUi(this);
  initializeContainer();
  // QImage image(":/images/icon.png");

  // QGraphicsScene* scene = new QGraphicsScene();
  // ui->graphicsView->setScene(scene);
  // QGraphicsPixmapItem* item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
  // scene->addItem(item);

  // ui->graphicsView->show();
}

AboutWindow::~AboutWindow() {
  delete ui;
}
