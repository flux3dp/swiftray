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

  void setLayout();

  void setFont(const QFont &font);

  void setPointSize(int point_size);

  void setLetterSpacing(double spacing);

  void setBold(bool bold);

  void setItalic(bool italic);

  void setUnderline(bool underline);

  void setLineHeight(double line_height);

private:
  void loadStyles() override;

  void registerEvents() override;

  void setFont(QFont font, float line_height);

  void hideEvent(QHideEvent *event) override;
  
  void showEvent(QShowEvent *event) override;

  Ui::FontPanel *ui;
  MainWindow *main_window_;
  QFont font_;
  double line_height_;

signals:
  void lineHeightChanged(double line_height);

  void fontChanged(QFont font);

  void fontPointSizeChanged(int point_size);

  void fontLetterSpacingChanged(double letter_spacing);

  void fontBoldChanged(bool font_bold);

  void fontItalicChanged(bool font_italic);

  void fontUnderlineChanged(bool font_underline);

  void panelShow(bool is_show);
};

#endif // FONTPANEL_H
