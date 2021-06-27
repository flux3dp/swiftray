#ifndef NEWMACHINEDIALOG_H
#define NEWMACHINEDIALOG_H

#include <QDialog>
#include <settings/machine-settings.h>

namespace Ui {
  class NewMachineDialog;
}

class NewMachineDialog : public QDialog {
Q_OBJECT

public:
  explicit NewMachineDialog(QWidget *parent = nullptr);

  ~NewMachineDialog();

  void loadSettings();

  void registerEvents();

  MachineSettings::MachineSet machine() const;

private:
  Ui::NewMachineDialog *ui;
};

#endif // NEWMACHINEDIALOG_H
