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

  Ui::TransformPanel *ui;
  double x_;
  double y_;
  double r_;
  double w_;
  double h_;
  bool scale_locked_;
  MainWindow *main_window_;

signals:
  void transformPanelUpdated(double x, double y, double r, double w, double h);

  void scaleLockToggled(bool scale_locked);
};

#endif // TRANSFORM_WIDGET_H
