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

  double lineHeight();

  void setFont(QFont font, float line_height);

  void setLineHeight(double line_height);

private:
  void loadStyles() override;

  void registerEvents() override;

  Ui::FontPanel *ui;
  MainWindow *main_window_;
  QFont font_;

signals:
  void lineHeightChanged(double line_height);

  void fontSettingChanged();
};

#endif // FONTPANEL_H
