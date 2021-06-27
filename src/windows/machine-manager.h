#ifndef MACHINEMANAGER_H
#define MACHINEMANAGER_H

#include <QDialog>

namespace Ui {
  class MachineManager;
}

class MachineManager : public QDialog {
Q_OBJECT

public:
  explicit MachineManager(QWidget *parent = nullptr);

  ~MachineManager();

  void loadSettings();

  void loadStyles();

  void loadWidgets();

  void registerEvents();

  void save();

private:
  Ui::MachineManager *ui;
};

#endif // MACHINEMANAGER_H
