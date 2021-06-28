#include <QDebug>
#include <document.h>

/*
We use a *single file* for the serializer to maintain different file versions.

If we add new properties to the document/layer/shape objects in new versions, 
we can extend the serializer instead of adding a lot if/else to different classes.

We can also use JSON, which is easier for backward-compatiblity and debug, but the performance may be slow.

We may also use JSON partially to support volatile classes.

JSON (O): Document settings, memo, metadata
JSON (X): QPainterShape, QPixmap

*/

//TODO (Add JSON Serializer for debugging scenes)

class DocumentSerializer {
public:
  DocumentSerializer(QDataStream &stream) :
       out(stream),
       in(stream) {}

  void serializeDocument(Document &doc) {
    out << doc.layers().size();
    for (auto &layer : doc.layers()) {
      serializeLayer(layer);
    }
  }

  Document *deserializeDocument() {
    Document *doc = new Document;
    int layers_size;
    in >> layers_size;
    doc->layers_.clear();
    for (int i = 0; i < layers_size; i++) {
      LayerPtr layer = deserializeLayer();
      doc->addLayer(layer);
    }
    return doc;
  }

  void serializeLayer(const LayerPtr &layer) {
    out << layer->name_;
    out << layer->type_;
    out << layer->color_;

    out << layer->use_diode_;
    out << layer->is_visible_;
    out << layer->target_height_;
    out << layer->step_height_;
    out << layer->repeat_;
    out << layer->speed_;

    out << layer->children().size();
    for (auto &shape: layer->children()) {
      // TODO (Use string instead of enum int for future compatibility)
      // TODO (Add operator for shape and datastream)
      serializeShape(shape);
    }
  }

  LayerPtr deserializeLayer() {
    LayerPtr layer = make_shared<Layer>();
    in >> layer->name_;
    in >> layer->type_;
    in >> layer->color_;

    in >> layer->use_diode_;
    in >> layer->is_visible_;
    in >> layer->target_height_;
    in >> layer->step_height_;
    in >> layer->repeat_;
    in >> layer->speed_;

    int shape_size;
    in >> shape_size;
    qInfo() << "Parsing layer" << layer->name_ << "children" << shape_size;
    for (int i = 0; i < shape_size; i++) {
      auto shape = deserializeShape();
      shape->flushCache();
      layer->addShape(shape);
    }
    return layer;
  }

  void serializeShape(const ShapePtr &shape) {
    out << shape->type();
    //blabla
    switch (shape->type()) {
      case Shape::Type::Path:
        serializePathShape((PathShape *) shape.get());
        break;
      case Shape::Type::Text:
        serializeTextShape((TextShape *) shape.get());
        break;
      case Shape::Type::Bitmap:
        serializeBitmapShape((BitmapShape *) shape.get());
        break;
      case Shape::Type::Group:
        serializeGroupShape((GroupShape *) shape.get());
        break;
      default:
        break;
    }
  }

  ShapePtr deserializeShape() {
    Shape::Type type;
    in >> type;
    qInfo() << "Parsing shape" << (int) type;
    switch (type) {
      case Shape::Type::Path:
        return ShapePtr(deserializePathShape());
      case Shape::Type::Text:
        return ShapePtr(deserializeTextShape());
      case Shape::Type::Bitmap:
        return ShapePtr(deserializeBitmapShape());
      case Shape::Type::Group:
        return ShapePtr(deserializeGroupShape());
      default:
        break;
    }
    Q_ASSERT_X(false, "Deserialize", "Failed to parse shape type");
  }

  void serializeShapeProp(Shape *shape) {
    out << shape->rotation_;
    out << shape->transform_;
    out << shape->selected_;
  }

  void deserializeShapeProp(Shape *shape) {
    in >> shape->rotation_;
    in >> shape->transform_;
    in >> shape->selected_;
  }

  void serializeBitmapShape(BitmapShape *shape) {
    serializeShapeProp(shape);
    out << *shape->bitmap_.get();
  }

  BitmapShape *deserializeBitmapShape() {
    BitmapShape *shape = new BitmapShape();
    deserializeShapeProp(shape);
    QPixmap pixmap;
    in >> pixmap;
    shape->bitmap_ = make_unique<QPixmap>(pixmap);
    return shape;
  }


  void serializePathShape(PathShape *shape) {
    serializeShapeProp(shape);
    out << shape->filled_;
    out << shape->path_;
  }

  PathShape *deserializePathShape() {
    auto *shape = new PathShape();
    deserializeShapeProp(shape);
    in >> shape->filled_;
    in >> shape->path_;
    return shape;
  }


  void serializeGroupShape(GroupShape *shape) {
    serializeShapeProp(shape);
    out << shape->children_.size();
    for (auto &shape: shape->children_) {
      serializeShape(shape);
    }
  }

  GroupShape *deserializeGroupShape() {
    auto *group = new GroupShape();
    deserializeShapeProp(group);
    int children_size;
    in >> children_size;
    for (int i = 0; i < children_size; i++) {
      auto shape = deserializeShape();
      shape->setParent(group);
      shape->flushCache();
      group->children_ << shape;
    }
    return group;
  }

  void serializeTextShape(TextShape *shape) {
    serializeShapeProp(shape);
    out << shape->filled_;
    out << shape->path_;
    out << shape->line_height_;
    out << shape->lines_;
    out << shape->font_;
  }

  TextShape *deserializeTextShape() {
    auto *shape = new TextShape();
    deserializeShapeProp(shape);
    in >> shape->filled_;
    in >> shape->path_;
    in >> shape->line_height_;
    in >> shape->lines_;
    in >> shape->font_;
    return shape;
  }

  QDataStream &out;
  QDataStream &in;
};