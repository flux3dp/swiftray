#ifndef DOCSETTINGSPANEL_H
#define DOCSETTINGSPANEL_H

#include <QFrame>
#include <settings/machine-settings.h>
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

  void updateScene();

  MachineSettings::MachineSet currentMachine();

  QString getMachineName();

  void setRotaryMode(bool is_rotary_mode);

signals:
  void machineChanged(QString machine_name);
  void rotaryModeChange(bool is_rotary_mode);

private:
  void loadSettings() override;

  void registerEvents() override;

  void syncDPISettingsUI();

  void syncAdvancedSettingsUI();

  Ui::DocPanel *ui;
  MainWindow *main_window_;
};

#endif // DOCSETTINGSPANEL_H
