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

private:
  Ui::PreferencesWindow *ui;
};
