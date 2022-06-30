#pragma once

#include <QDialog>
#include <widgets/base-container.h>

namespace Ui {
  class AboutWindow;
}

class AboutWindow : public QDialog, BaseContainer {
Q_OBJECT

public:
  explicit AboutWindow(QWidget *parent = nullptr);

  ~AboutWindow();

private:
  Ui::AboutWindow *ui;
};
