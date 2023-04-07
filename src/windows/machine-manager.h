#pragma once

#include <QButtonGroup>
#include <QDialog>
#include <settings/machine-settings.h>
#include <widgets/base-container.h>

namespace Ui {
  class MachineManager;
}

class MachineManager : public QDialog, BaseContainer {
Q_OBJECT

public:
  explicit MachineManager(QWidget *parent, int machine_index);
  ~MachineManager();
  void setMachineIndex(int index);
  void setRotaryAxis(char rotary_axis);

Q_SIGNALS:
  void updateCurrentMachineIndex(int machine_index);

public Q_SLOTS:
  void show();

private:
  void loadSettings() override;
  void loadStyles() override;
  void loadWidgets() override;
  void registerEvents() override;
  void save();
  void originChanged(MachineSettings::MachineParam::OriginType origin);

  Ui::MachineManager *ui;
  QButtonGroup *axis_group_;
};
