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
  setShareComboBox();
  QFont current_font = QApplication::font();
  if(current_font.pixelSize() > 0) {
    ui->horizontalSliderFontSize->setValue(current_font.pixelSize());
    ui->labelFontSize->setText(QString::number(current_font.pixelSize()));
  }
  else {
    ui->horizontalSliderFontSize->setValue(13);
    ui->labelFontSize->setText(QString::number(13));
  }

  connect(ui->buttonBox, &QDialogButtonBox::accepted, [=](){
    QSettings settings("flux", "swiftray");
    QVariant language_code = settings.value("window/language", 0);
    QVariant font_size = settings.value("window/font_size", 0);
    if (language_code.toInt() != ui->comboBox->currentIndex() || font_size.toInt() != ui->horizontalSliderFontSize->value()) {
      settings.setValue("window/language", ui->comboBox->currentIndex());
      settings.setValue("window/font_size", ui->horizontalSliderFontSize->value());
      QMessageBox msgbox;
      msgbox.setInformativeText(tr("Please restart Swiftray to enable new settings."));
      msgbox.exec();
    }
    Q_EMIT speedModeChanged(ui->comboBoxSpeedOptimization->currentIndex());
    Q_EMIT privacyUpdate(ui->comboBoxShare->currentIndex());
  });
  connect(ui->horizontalSliderFontSize, &QAbstractSlider::valueChanged, [=](int value){
    ui->labelFontSize->setText(QString::number(value));
    QFont current_font = QApplication::font();
    current_font.setPixelSize(value);
    ui->labelFontSize->setFont(current_font);
    QApplication::setFont(current_font);
  });
}

void PreferencesWindow::setSpeedMode(bool is_high_speed) {
  ui->comboBoxSpeedOptimization->blockSignals(true);
  ui->comboBoxSpeedOptimization->setCurrentIndex(is_high_speed);
  ui->comboBoxSpeedOptimization->blockSignals(false);
}

void PreferencesWindow::setUpload(bool enable_upload) {
  ui->comboBoxShare->blockSignals(true);
  ui->comboBoxShare->setCurrentIndex(enable_upload);
  ui->comboBoxShare->blockSignals(false);
}

void PreferencesWindow::setLanguageComboBox() {
  ui->comboBox->addItem("English");
  // \u4e2d\u6587 為“中文”unicode
  ui->comboBox->addItem(QString::fromUtf16(u"\u4e2d\u6587"));
  // \u65e5\u672c\u8a9e 為“日本語”unicode
  ui->comboBox->addItem(QString::fromUtf16(u"\u65e5\u672c\u8a9e"));
  QSettings settings("flux", "swiftray");
  QVariant language_code = settings.value("window/language", 0);
  ui->comboBox->setCurrentIndex(language_code.toInt());
}

void PreferencesWindow::setSpeedOptimizationComboBox() {
  ui->comboBoxSpeedOptimization->addItem("Off");
  ui->comboBoxSpeedOptimization->addItem("On");
}

void PreferencesWindow::setShareComboBox() {
  ui->comboBoxShare->addItem("Off");
  ui->comboBoxShare->addItem("On");
}

void PreferencesWindow::setTabWidget() {
  ui->tabWidget->removeTab(1); // hide advance tab temporarily
}

PreferencesWindow::~PreferencesWindow() {
  delete ui;
}
