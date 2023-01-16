#ifndef FONTPANEL_H
#define FONTPANEL_H

#include <QFrame>
#include <QFont>
#include <QSet>
#include <widgets/base-container.h>

class MainWindow;

namespace Ui {
  class FontPanel;
}

class FontPanel : public QFrame, BaseContainer {
Q_OBJECT

public:
  explicit FontPanel(QWidget *parent, bool is_dark_mode);
  ~FontPanel();
  void setFontFamily(QString font_family);
  void setPointSize(int point_size);
  void setLetterSpacing(double spacing);
  void setBold(bool bold);
  void setItalic(bool italic);
  void setUnderline(bool underline);
  void setLineHeight(double line_height);

public Q_SLOTS:
  void updateFontView(QSet<QString> font_familys, 
                      QSet<int> point_sizes, 
                      QSet<qreal> letter_spacings, 
                      QSet<bool> bolds, 
                      QSet<bool> italics, 
                      QSet<bool> underlines, 
                      QSet<double> line_heights);

private:
  void loadStyles() override;
  void registerEvents() override;
  void hideEvent(QHideEvent *event) override;
  void showEvent(QShowEvent *event) override;
  void setLayout(bool is_dark_mode);

  Ui::FontPanel *ui;

Q_SIGNALS:
  void editShapeFontFamily(QFont font);
  void editShapeFontPointSize(int point_size);
  void editShapeLetterSpacing(qreal letter_spacing);
  void editShapeBold(bool bold);
  void editShapeItalic(bool italic);
  void editShapeUnderline(bool underline);
  void editShapeLineHeight(double line_height);
  void panelShow(bool is_show);
};

#endif // FONTPANEL_H
