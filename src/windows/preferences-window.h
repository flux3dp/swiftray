#pragma once

#include <QDialog>
#include <widgets/base-container.h>

namespace Ui {
  class PreferencesWindow;
}

class PreferencesWindow : public QDialog, BaseContainer {
Q_OBJECT

public:
  explicit PreferencesWindow(QWidget *parent = nullptr);

  ~PreferencesWindow();

  bool isHighSpeedMode();

private:
  Ui::PreferencesWindow *ui;

  void setLanguageComboBox();

  void setSpeedOptimizationComboBox();

  void setTabWidget();

signals:
  void setSpeedMode(bool is_high_speed);
};
