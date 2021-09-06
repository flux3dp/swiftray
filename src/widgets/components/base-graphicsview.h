#pragma once

#include <QGraphicsView>
#include <QGestureEvent>

class BaseGraphicsView: public QGraphicsView {
public:
    BaseGraphicsView(QWidget *parent = nullptr);
    BaseGraphicsView(QGraphicsScene *scene = nullptr, QWidget *parent = nullptr);
    bool event(QEvent *e) override;

    void setMinScale(qreal min_scale) { min_scale_ = min_scale; }
protected:
    void gestureHandler(QGestureEvent *ge);
    void pinchGestureHandler(QPinchGesture *pg);

    qreal scaleFactor = 1;
    qreal min_scale_ = 1;
    bool zooming_ = false;
    QPointF zoom_fixed_point_scene_; // the cursor location in scene when start zooming
    QPointF zoom_fixed_point_view_;  // initial zoom cursor location in view coord
};