#ifndef DOCSETTINGSPANEL_H
#define DOCSETTINGSPANEL_H

#include <QFrame>
#include <widgets/base-container.h>

class MainWindow;

namespace Ui {
  class DocPanel;
}

class DocPanel : public QFrame, BaseContainer {
Q_OBJECT

public:
  explicit DocPanel(QWidget *parent, MainWindow *main_window);
  ~DocPanel();
  void setMachineIndex(int machine_index);
  void setRotaryMode(bool is_rotary_mode);
  void setPresetIndex(int preset_index);
  void setTravelSpeed(double travel_speed);
  void setRotarySpeed(double rotary_speed);
  void setMachineSelectLock(bool enable);
  void setPresetSelectLock(bool enable);

Q_SIGNALS:
  void updatePresetIndex(int index);
  void updateMachineIndex(int index);
  void updateTravelSpeed(double travel_speed);
  void updateRotarySpeed(double rotary_speed);
  void rotaryModeChange(bool is_rotary_mode);
  void panelShow(bool is_show);

private:
  void loadSettings() override;
  void registerEvents() override;
  void syncDPISettingsUI();
  void syncAdvancedSettingsUI();
  void hideEvent(QHideEvent *event) override;
  void showEvent(QShowEvent *event) override;

  Ui::DocPanel *ui;
  MainWindow *main_window_;
};

#endif // DOCSETTINGSPANEL_H
