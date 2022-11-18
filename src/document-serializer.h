#include <QDebug>
#include <document.h>
#include <QJsonObject>

/**
 *  \class DocumentSerializer
 *  \brief Save and load documents as binary format.
 * 
 *  We use a *single file* for the serializer to maintain different file versions,
 *  so if we add new properties to the document/layer/shape objects in new versions,
 *  we can extend the serializer instead of adding a lot if/else to different classes.
 *  Notes: We can also use JSON, which is easier for backward-compatiblity and debug, but the performance may be slow.
 *  Notes: We may also use JSON partially to support volatile classes.
 *  JSON (O): Document settings, memo, metadata
 *  JSON (X): QPainterShape, QPixmap
 *  TODO (Add JSON Serializer for debugging scenes)
*/

class DocumentSerializer {
public:
  DocumentSerializer(QDataStream &stream) :
       out(stream),
       in(stream) {}

  void serializeDocument(Document &doc) {
    out << QString("NINJAV1.3");
    out << QSize(doc.width(), doc.height());
    out << doc.layers().size();
    for (auto &layer : doc.layers()) {
      serializeLayer(layer);
    }

    QJsonObject doc_settings_json = {
        {"machine_model", doc.settings().machine_model},
        {"dpi", doc.settings().dpi},
        { "advanced",
          QJsonObject {
            {"use_af", doc.settings().use_af},
            {"use_diode", doc.settings().use_diode},
            {"use_rotary", doc.settings().use_rotary},
            {"use_open_bottom", doc.settings().use_open_bottom}
          }
        }
    };
    out << doc_settings_json;

  }

  Document *deserializeDocument() {
    QString doc_version;
    in >> doc_version;
    qInfo() << "Doc Version" << doc_version;
    if(doc_version == "NINJAV1.2") 
      version_index_ = NINJAV1_2;
    else if(doc_version == "NINJAV1.1")
      version_index_ = NINJAV1_1;
    else if(doc_version == "NINJAV1.3")
      version_index_ = NINJAV1_3;
    else
      return nullptr;
    Document *doc = new Document;
    QSize doc_size;
    in >> doc_size;
    doc->setWidth(doc_size.width());
    doc->setHeight(doc_size.height());

    int layers_size;
    in >> layers_size;
    doc->layers_.clear();
    for (int i = 0; i < layers_size; i++) {
      LayerPtr layer = deserializeLayer();
      doc->addLayer(layer);
    }

    QJsonObject doc_settings_obj;
    in >> doc_settings_obj;
    qInfo() << "Doc settings: " << doc_settings_obj;
    if ( doc_settings_obj.contains("machine_model") && doc_settings_obj.value("machine_model").isString() ) {
      doc->settings().machine_model = doc_settings_obj.value("machine_model").toString();
    }
    if ( doc_settings_obj.contains("dpi") && doc_settings_obj.value("dpi").isDouble() ) {
      doc->settings().dpi = doc_settings_obj.value("dpi").toDouble();
    }
    if ( doc_settings_obj.contains("advanced") && doc_settings_obj.value("advanced").isObject() ) {
      QJsonObject advanced_setting_obj = doc_settings_obj.value("advanced").toObject();
      if ( advanced_setting_obj.contains("use_af") && advanced_setting_obj.value("use_af").isBool() ) {
        doc->settings().use_af = advanced_setting_obj.value("use_af").toBool();
      }
      if ( advanced_setting_obj.contains("use_diode") && advanced_setting_obj.value("use_diode").isBool() ) {
        doc->settings().use_diode = advanced_setting_obj.value("use_diode").toBool();
      }
      if ( advanced_setting_obj.contains("use_rotary") && advanced_setting_obj.value("use_rotary").isBool() ) {
        doc->settings().use_rotary = advanced_setting_obj.value("use_rotary").toBool();
      }
      if ( advanced_setting_obj.contains("use_open_bottom") && advanced_setting_obj.value("use_open_bottom").isBool() ) {
        doc->settings().use_open_bottom = advanced_setting_obj.value("use_open_bottom").toBool();
      }
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
    out << layer->power_;
    out << layer->speed_;
    out << layer->x_backlash_;

    out << layer->children().size();
    for (auto &shape: layer->children()) {
      // TODO (Use string instead of enum int for future compatibility)
      // TODO (Add operator for shape and datastream)
      serializeShape(shape);
    }
  }

  LayerPtr deserializeLayer() {
    LayerPtr layer = std::make_shared<Layer>();
    in >> layer->name_;
    in >> layer->type_;
    // remove the index bigger than FillLine(2), and replace it to Fill(1)
    if((int)layer->type_ >= 2) layer->type_ = (Layer::Type)1;
    in >> layer->color_;

    in >> layer->use_diode_;
    in >> layer->is_visible_;
    in >> layer->target_height_;
    in >> layer->step_height_;
    in >> layer->repeat_;
    if(version_index_ >= NINJAV1_2) {
      in >> layer->power_;
      in >> layer->speed_;
    }
    else {
      int temp;
      in >> temp;
      layer->power_ = temp;
      in >> temp;
      layer->speed_ = temp;
    }
    if(version_index_ >= NINJAV1_3) {
      in >> layer->x_backlash_;
    }

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
    out << shape->src_image_;
    out << shape->gradient_;
    out << shape->thrsh_brightness_;
  }

  BitmapShape *deserializeBitmapShape() {
    BitmapShape *shape = new BitmapShape();
    deserializeShapeProp(shape);
    in >> shape->src_image_;
    in >> shape->gradient_;
    in >> shape->thrsh_brightness_;
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
  int version_index_;
  enum version_id
  {
    NINJAV1_1 = 0,
    NINJAV1_2 ,//change layer->power, layer->speed to double
    NINJAV1_3 //add layer->x_backlash
  };
};
