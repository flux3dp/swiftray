#pragma once

#include <QDialog>
#include <QQuickItem>
#include <QQuickWidget>
#include <settings/machine-settings.h>

class MainWindow;

class WelcomeDialog : public QDialog {
Q_OBJECT
public:
  explicit WelcomeDialog(QWidget *parent);
  ~WelcomeDialog() = default;
  void setupMachine();

Q_SIGNALS:
  void addNewMachine(MachineSettings::MachineParam new_param);

public Q_SLOTS:
  void createStandardProfile(const QString brand, const QString model);
  void createOtherProfile(const QString name, int width, int height, int origin);
  void close();

private:
  QQuickWidget *widget_;
};