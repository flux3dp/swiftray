#ifndef TRANSFORM_WIDGET_H
#define TRANSFORM_WIDGET_H

#include <QFrame>
#include <QDebug>
#include <shape/shape.h>
#include <canvas/controls/transform.h>
#include <widgets/base-container.h>

class MainWindow;

namespace Ui {
  class TransformPanel;
}

class TransformPanel : public QFrame, BaseContainer {
Q_OBJECT

public:
  explicit TransformPanel(QWidget *parent, MainWindow *main_window);

  ~TransformPanel();

  bool isScaleLock() const;

  void setLayout();

  void setScaleLock(bool scaleLock);

  void updateControl();

private:
  void loadStyles() override;

  void registerEvents() override;

  void hideEvent(QHideEvent *event) override;

  void showEvent(QShowEvent *event) override;

  Ui::TransformPanel *ui;
  double x_;
  double y_;
  double r_;
  double w_;
  double h_;
  bool scale_locked_;
  MainWindow *main_window_;

Q_SIGNALS:
  void transformPanelUpdated(double x, double y, double r, double w, double h);

  void scaleLockToggled(bool scale_locked);

  void panelShow(bool is_show);
};

#endif // TRANSFORM_WIDGET_H
