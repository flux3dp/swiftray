#pragma once

#include <widgets/components/base-graphicsview.h>
#include <QGraphicsView>
#include <QGestureEvent>
#include <QGraphicsItem>

class ImageTraceGraphicsView: public BaseGraphicsView {
  Q_OBJECT

public:
    /**
     * @brief Used to draw trace path and points on it
     *        NOTE: Can be extracted to a separate file
     */
    class QGraphicsContourPathsItem: public QGraphicsPathItem {
    public:
        QGraphicsContourPathsItem();
        QGraphicsContourPathsItem(const QPainterPath &path, QGraphicsItem *parent = nullptr);
        QGraphicsContourPathsItem(QGraphicsItem *parent = nullptr);

        QRectF boundingRect() const;
        void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
        void setShowPoints(bool enable) { show_points_ = enable; }
    private:
        bool show_points_ = false;
    };

    ImageTraceGraphicsView(QWidget *parent = nullptr);
    void reset() override;
    void updateBackgroundPixmap(QPixmap background_img) override;
    void updateTrace(const QPainterPath& contours);
    void setShowPoints(bool enable);
    void clearSelectionArea();
    void drawSelectionArea();

public Q_SLOTS:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

Q_SIGNALS:
    void selectionAreaChanged();

private:
    constexpr static int ITEM_ID_KEY = 0;

    constexpr static int BACKGROUND_IMAGE_Z_INDEX = 0;
    constexpr static char BACKGROUND_IMAGE_ITEM_ID[] = "BACKGROUND";
    constexpr static int SELECTION_RECT_Z_INDEX = 1;
    constexpr static char SELECTION_RECT_ITEM_ID[] = "SELECT_RECT";
    constexpr static int IMAGE_TRACE_Z_INDEX = 2;
    constexpr static char IMAGE_TRACE_ITEM_ID[] = "TRACE";

    // Might replace the following with a template function
    QGraphicsRectItem* getSelectionAreaRectItem();
    QGraphicsContourPathsItem* getTraceContourPathsItem();
};