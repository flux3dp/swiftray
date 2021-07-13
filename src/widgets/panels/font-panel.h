#ifndef FONTPANEL_H
#define FONTPANEL_H

#include <QFrame>
#include <QFont>
#include <widgets/base-container.h>

class MainWindow;

namespace Ui {
  class FontPanel;
}

class FontPanel : public QFrame, BaseContainer {
Q_OBJECT

public:
  explicit FontPanel(QWidget *parent, MainWindow *main_window);

  ~FontPanel();

  QFont font();

  void setFont(QFont font, float line_height);

private:
  void loadStyles() override;

  void registerEvents() override;

  Ui::FontPanel *ui;
  MainWindow *main_window_;
  QFont font_;
};

#endif // FONTPANEL_H
