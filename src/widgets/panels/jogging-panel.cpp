#include "jogging-panel.h"
#include "ui_jogging-panel.h"
#include <QQuickItem>
#include <QQuickWidget>
#include <settings/machine-settings.h>
#include <windows/mainwindow.h>
#include <windows/osxwindow.h>
#include <globals.h>

JoggingPanel::JoggingPanel(QWidget *parent, MainWindow *main_window) :
     QFrame(parent),
     main_window_(main_window),
     ui(new Ui::JoggingPanel),
     BaseContainer() {
  assert(parent != nullptr && main_window != nullptr);
  ui->setupUi(this);
  initializeContainer();
  ui->maintenanceController->setSource(QUrl("qrc:/src/windows/qml/MaintenanceDialog.qml"));
  ui->maintenanceController->rootContext()->setContextProperty("is_dark_mode", isDarkMode());
  ui->maintenanceController->show();
  ui->laserBtn->setText(tr("Laser On"));

  // Handle event from QML UI
  QObject::connect(ui->maintenanceController->rootObject(), SIGNAL(moveRelatively(int, int)),
                  this, SLOT(moveRelatively(int, int)));
  QObject::connect(ui->maintenanceController->rootObject(), SIGNAL(moveToCorner(int)),
                  this, SLOT(moveToCorner(int)));
  QObject::connect(ui->maintenanceController->rootObject(), SIGNAL(moveToEdge(int)),
                  this, SLOT(moveToEdge(int)));
  QObject::connect(ui->maintenanceController->rootObject(), SIGNAL(home()),
                  this, SLOT(home()));
  QObject::connect(ui->laserBtn, &QAbstractButton::clicked, this, &JoggingPanel::laser);
  QObject::connect(ui->laserPulseBtn, &QAbstractButton::clicked, this, &JoggingPanel::laserPulse);
  connect(ui->syncBtn, &QAbstractButton::clicked, [=]() {
    ui->moveXSpinBox->setValue(ui->currentXSpinBox->value());
    ui->moveYSpinBox->setValue(ui->currentYSpinBox->value());
  });
  connect(ui->goBtn, &QAbstractButton::clicked, [=]() {
    moveAbsolutely(
      std::make_tuple<qreal, qreal, qreal>(ui->moveXSpinBox->value(), ui->moveYSpinBox->value(), 0)
    );
  });
  connect(ui->setOriginBtn, &QAbstractButton::clicked, this, &JoggingPanel::setOrigin);
  connect(ui->clearOriginBtn, &QAbstractButton::clicked, this, &JoggingPanel::clearOrigin);

  // Receive signals from mainwindow
  connect(main_window_, &MainWindow::positionCached, this, &JoggingPanel::updateCurrentPos);
} 

void JoggingPanel::home() {
  if(!control_enable_) {
    return;
  }
  emit actionHome();
}

/**
 * @brief Toggle laser emit state
 * 
 */
void JoggingPanel::laser() {
  if(!control_enable_) {
    return;
  }
  if (is_laser_on_) {
    ui->laserBtn->setText(tr("Laser On"));
    is_laser_on_ = false;
    emit actionLaser(0); // turn off laser
  } else {
    ui->laserBtn->setText(tr("Laser Off"));
    is_laser_on_ = true;
    emit actionLaser(ui->laserSpinBox->value()); // default: 2%
  }
}

void JoggingPanel::laserPulse() {
  if(!control_enable_) {
    return;
  }
  emit actionLaserPulse(ui->laserPulseSpinBox->value()); // default: 30%
}

/**
 * @brief 
 * 
 * @param dir   0: move right (in canvas coord)
 *              1: move up    (in canvas coord)
 *              2: move left  (in canvas coord)
 *              3: move down  (in canvas coord)
 * @param level 0: 0.1mm
 *              1: 2 mm
 *              2: 10mm
 */
void JoggingPanel::moveRelatively(int dir, int level) {
  if(!control_enable_) {
    return;
  }
  float magnitude = 0;
  QPointF movement(0,0);

  switch (level) {
    case 0:
      magnitude = 0.1;
      break;
    case 1:
      magnitude = 2;
      break;
    case 2:
      magnitude = 10;
      break;
  }

  switch (dir) {
    case 0:
      movement.setX(magnitude);
      break;
    case 1:
      movement.setY(-magnitude);
      break;
    case 2:
      movement.setX(-magnitude);
      break;
    case 3:
      movement.setY(magnitude);
      break;
  }

  // TODO: Get feedrate from UI?
  emit actionMoveRelatively(movement.x(), movement.y(), travel_speed_);
}

void JoggingPanel::moveToEdge(int edge_id) {
  if(!control_enable_) {
    return;
  }
  // TODO: Get feedrate from UI?
  emit actionMoveToEdge(edge_id, travel_speed_);
}

void JoggingPanel::moveToCorner(int corner_id) {
  if(!control_enable_) {
    return;
  }
  // TODO: Get feedrate from UI?
  emit actionMoveToCorner(corner_id, travel_speed_);
}

void JoggingPanel::moveAbsolutely(std::tuple<qreal, qreal, qreal> pos) {
  if(!control_enable_) {
    return;
  }
  emit actionMoveAbsolutely(pos, travel_speed_);
}

void JoggingPanel::setOrigin() {
  if(!control_enable_) {
    return;
  }
  emit actionSetOrigin(
    std::make_tuple<qreal, qreal, qreal>(
      ui->currentXSpinBox->value(), 
      ui->currentYSpinBox->value(), 
      0)
  );
}

void JoggingPanel::clearOrigin() {
  emit actionSetOrigin(std::make_tuple<qreal, qreal, qreal>(0, 0, 0));
}

void JoggingPanel::setControlEnable(bool control_enable) {
  control_enable_ = control_enable;
  ui->goBtn->setEnabled(control_enable);
  ui->setOriginBtn->setEnabled(control_enable);
  ui->laserBtn->setEnabled(control_enable);
  ui->laserPulseBtn->setEnabled(control_enable);
}

void JoggingPanel::hideEvent(QHideEvent *event) {
  emit panelShow(false);
}

void JoggingPanel::showEvent(QShowEvent *event) {
  emit panelShow(true);
}

void JoggingPanel::updateCurrentPos(std::tuple<qreal, qreal, qreal> pos) {
  ui->currentXSpinBox->setValue(std::get<0>(pos));
  ui->currentYSpinBox->setValue(std::get<1>(pos));
}

JoggingPanel::~JoggingPanel() {
  delete ui;
}
