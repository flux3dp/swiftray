#ifndef FONTPANEL_H
#define FONTPANEL_H

#include <QFrame>
#include <QFont>

class MainWindow;

namespace Ui {
  class FontPanel;
}

class FontPanel : public QFrame {
Q_OBJECT

public:
  explicit FontPanel(QWidget *parent, MainWindow *main_window);

  ~FontPanel();

  void loadStyles();

  void registerEvents();

  QFont font();

  void setFont(QFont font, float line_height);

private:
  Ui::FontPanel *ui;
  MainWindow *main_window_;
  QFont font_;
};

#endif // FONTPANEL_H
