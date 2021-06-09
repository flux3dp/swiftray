#include <QList>
#include <QPainter>
#include <canvas/layer.h>
#include <shape/bitmap_shape.h>
#include <shape/path_shape.h>
#include <shape/group_shape.h>

class GCodeGenerator {
    public:
        GCodeGenerator() noexcept;
        void convertStack(const QList<LayerPtr> &layers);
    private:
        void convertLayer(const LayerPtr &layer);
        void convertShape(const ShapePtr &shape);
        void convertGroup(const GroupShape* group);
        void convertBitmap(const BitmapShape* bmp);
        void convertPath(const PathShape* path);
            
        void sortPolygons();
        void outputLayerGcode();
        void outputLayerPathGcode();
        void outputLayerBitmapGcode();
        QList<QString> gcode_;
        QTransform global_transform_;
        QList<ShapePtr> layer_elements_;
        QList<QPolygonF> layer_polygons_;
        QPixmap layer_bitmap_;
        unique_ptr<QPainter> layer_painter_;
};