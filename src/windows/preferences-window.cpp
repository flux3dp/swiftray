#include<QDebug>
#include <QMessageBox>
#include <QSettings>
#include "preferences-window.h"
#include "ui_preferences-window.h"

PreferencesWindow::PreferencesWindow(QWidget *parent) :
     QDialog(parent),
     ui(new Ui::PreferencesWindow),
     BaseContainer() {
  ui->setupUi(this);
  initializeContainer();
  setTabWidget();
  setLanguageComboBox();
  setSpeedOptimizationComboBox();

  connect(ui->buttonBox, &QDialogButtonBox::accepted, [=](){
    QSettings settings;
    QVariant language_code = settings.value("window/language", 0);
    if (language_code.toInt() != ui->comboBox->currentIndex()) {
      settings.setValue("window/language", ui->comboBox->currentIndex());
      QMessageBox msgbox;
      msgbox.setInformativeText(tr("Please restart Swiftray to enable new settings."));
      msgbox.exec();
    }
    emit setSpeedMode(ui->comboBoxSpeedOptimization->currentIndex());
  });
}

bool PreferencesWindow::isHighSpeedMode() {
  return ui->comboBoxSpeedOptimization->currentIndex();
}

void PreferencesWindow::setLanguageComboBox() {
  ui->comboBox->addItem("English");
  ui->comboBox->addItem("中文");
  QSettings settings;
  QVariant language_code = settings.value("window/language", 0);
  ui->comboBox->setCurrentIndex(language_code.toInt());
}

void PreferencesWindow::setSpeedOptimizationComboBox() {
  ui->comboBoxSpeedOptimization->addItem("Off");
  ui->comboBoxSpeedOptimization->addItem("On");
  ui->comboBoxSpeedOptimization->setCurrentIndex(0);
}

void PreferencesWindow::setTabWidget() {
  ui->tabWidget->removeTab(1); // hide advance tab temporarily
}

PreferencesWindow::~PreferencesWindow() {
  delete ui;
}
