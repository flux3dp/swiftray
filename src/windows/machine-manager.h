#pragma once

#include <QDialog>
#include <settings/machine-settings.h>
#include <widgets/base-container.h>

class MainWindow;

namespace Ui {
  class MachineManager;
}

class MachineManager : public QDialog, BaseContainer {
Q_OBJECT

public:

  explicit MachineManager(QWidget *parent);

  ~MachineManager();

  void save();

private slots:

  void originChanged(MachineSettings::MachineSet::OriginType origin);

private:

  void loadSettings() override;

  void loadStyles() override;

  void loadWidgets() override;

  void registerEvents() override;

  Ui::MachineManager *ui;
};
