#include "preferences-window.h"
#include "ui_preferences-window.h"

PreferencesWindow::PreferencesWindow(QWidget *parent) :
     QDialog(parent),
     ui(new Ui::PreferencesWindow),
     BaseContainer() {
  ui->setupUi(this);
  initializeContainer();
}

PreferencesWindow::~PreferencesWindow() {
  delete ui;
}
