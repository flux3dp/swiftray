#ifndef SPOOLERPANEL_H
#define SPOOLERPANEL_H

#include <QFrame>
#include <widgets/base-container.h>

namespace Ui {
  class SpoolerPanel;
}

class SpoolerPanel : public QFrame, BaseContainer {
Q_OBJECT

public:
  explicit SpoolerPanel(QWidget *parent = nullptr);

  ~SpoolerPanel();

private:
  Ui::SpoolerPanel *ui;
};

#endif // SPOOLERPANEL_H
