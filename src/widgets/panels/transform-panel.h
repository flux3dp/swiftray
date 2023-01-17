#ifndef TRANSFORM_WIDGET_H
#define TRANSFORM_WIDGET_H

#include <QFrame>
#include <QDebug>
#include <widgets/base-container.h>

class MainWindow;

namespace Ui {
  class TransformPanel;
}

class TransformPanel : public QFrame, BaseContainer {
Q_OBJECT

public:
  explicit TransformPanel(QWidget *parent, bool is_dark_mode);
  ~TransformPanel();
  void setLayout(bool is_dark_mode);
  void setTransformX(double x);
  void setTransformY(double y);
  void setTransformR(double r);
  void setTransformW(double w);
  void setTransformH(double h);
  void setScaleLock(bool scaleLock);

private:
  void loadStyles() override;
  void registerEvents() override;
  void hideEvent(QHideEvent *event) override;
  void showEvent(QShowEvent *event) override;

  Ui::TransformPanel *ui;
  bool is_dark_mode_;

Q_SIGNALS:
  void editShapeTransformX(double x);
  void editShapeTransformY(double y);
  void editShapeTransformR(double r);
  void editShapeTransformW(double w);
  void editShapeTransformH(double h);
  void scaleLockToggled(bool scale_locked);
  void panelShow(bool is_show);
};

#endif // TRANSFORM_WIDGET_H
