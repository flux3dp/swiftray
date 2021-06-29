#ifndef TRANSFORM_WIDGET_H
#define TRANSFORM_WIDGET_H

#include <QFrame>
#include <QDebug>
#include <shape/shape.h>
#include <canvas/controls/transform.h>

class MainWindow;

namespace Ui {
  class TransformPanel;
}

class TransformPanel : public QFrame {
Q_OBJECT

public:
  explicit TransformPanel(QWidget *parent, MainWindow *main_window);

  ~TransformPanel();

  bool isScaleLock() const;

  void setScaleLock(bool scaleLock);

  void updateControl();

private:
  void loadStyles();

  void registerEvents();

  Ui::TransformPanel *ui;
  double x_;
  double y_;
  double r_;
  double w_;
  double h_;
  bool scale_locked_;
  MainWindow *main_window_;
};

#endif // TRANSFORM_WIDGET_H
