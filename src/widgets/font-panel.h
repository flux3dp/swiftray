#ifndef FONTPANEL_H
#define FONTPANEL_H

#include <QWidget>
#include <QFont>

class Canvas;

namespace Ui {
  class FontPanel;
}

class FontPanel : public QWidget {
Q_OBJECT

public:
  explicit FontPanel(QWidget *parent, Canvas *canvas);

  ~FontPanel();

  void loadStyles();

  void registerEvents();

  QFont font();

  void setFont(QFont font, float line_height);

private:
  Ui::FontPanel *ui;
  Canvas *canvas_;
  QFont font_;
};

#endif // FONTPANEL_H
