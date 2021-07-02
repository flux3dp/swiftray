#ifndef MACHINEMANAGER_H
#define MACHINEMANAGER_H

#include <QDialog>
#include <settings/machine-settings.h>

class MainWindow;

namespace Ui {
  class MachineManager;
}

class MachineManager : public QDialog {
Q_OBJECT

public:
  explicit MachineManager(QWidget *parent, MainWindow *main_window);

  ~MachineManager();

  void loadSettings();

  void loadStyles();

  void loadWidgets();

  void registerEvents();

  void save();

private slots:

  void originChanged(MachineSettings::MachineSet::OriginType origin);

private:
  Ui::MachineManager *ui;
  MainWindow *main_window_;
};

#endif // MACHINEMANAGER_H
