#ifndef DOCSETTINGSPANEL_H
#define DOCSETTINGSPANEL_H

#include <QFrame>

namespace Ui {
  class DocumentSettingsPanel;
}

class DocumentSettingsPanel : public QFrame {
Q_OBJECT

public:
  explicit DocumentSettingsPanel(QWidget *parent = nullptr);

  ~DocumentSettingsPanel();

private:
  Ui::DocumentSettingsPanel *ui;
};

#endif // DOCSETTINGSPANEL_H
