#ifndef NEWMACHINEDIALOG_H
#define NEWMACHINEDIALOG_H

#include <QDialog>
#include <settings/machine-settings.h>
#include <widgets/base-container.h>

namespace Ui {
  class NewMachineDialog;
}

class NewMachineDialog : public QDialog, BaseContainer {
Q_OBJECT

public:
  explicit NewMachineDialog(QWidget *parent = nullptr);

  ~NewMachineDialog();

  MachineSettings::MachineSet machine() const;

private:

  void loadSettings() override;

  void registerEvents() override;

  Ui::NewMachineDialog *ui;
};

#endif // NEWMACHINEDIALOG_H
