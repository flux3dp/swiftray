#pragma once

#include <widgets/components/base-graphicsview.h>
#include <QGraphicsView>
#include <QGestureEvent>
#include <QxPotrace/include/qxpotrace.h>


class ImageTraceGraphicsView: public BaseGraphicsView {
  Q_OBJECT

public:
    enum BackgroundDisplayMode {
        kGrayscale,
        kBinarized,
        kFaded
    };

    ImageTraceGraphicsView(QWidget *parent = nullptr);
    ImageTraceGraphicsView(QGraphicsScene *scene = nullptr, QWidget *parent = nullptr);
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void clearPartialSelect() {
      QPainterPath empty_path;
      this->scene()->setSelectionArea(empty_path);
    }

    void setImage(QImage src_img);
    void setThresholds(int low_thres, int high_thres) {
      low_thres_ = low_thres;
      high_thres_ = high_thres;
    }
    void updateBackgroundImage(BackgroundDisplayMode mode);
    void updateTrace(int cutoff, int threshold, int turd_size, qreal smooth, qreal optimize);
signals:
    void rubberBandSelect(QRectF selectionRect);

private:
    QImage src_image_grayscale_;
    int low_thres_;
    int high_thres_;
    QGraphicsRectItem* selection_area_rect_item_ = nullptr;
    QGraphicsPixmapItem* selection_area_pixmap_item_ = nullptr;
    QGraphicsPathItem* contours_path_item_ = nullptr;
    QGraphicsPixmapItem* bg_image_item_ = nullptr;
    std::shared_ptr<QxPotrace> potrace_;
    QImage ImageBinarize(const QImage &image,int threshold, int cutoff);
    QImage createSubImage(QImage* image, const QRect & rect);
};