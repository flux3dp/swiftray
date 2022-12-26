#include "dxf_reader.h"
#include "parser/dxf_rs/engine/rs_graphic.h"
#include "libdxfrw/drw_base.h"
#include <shape/bitmap-shape.h>

#include <QDebug>
#include <iostream>

DXFReader::DXFReader() {
}

LayerPtr DXFReader::findLayer(QString layer_name, QColor color) {
  LayerPtr target_layer;
  switch(read_type_) {
    case InSingleLayer:
      if(dxf_layers_.empty()) {
        dxf_layers_.push_back(std::make_shared<Layer>());
      }
      target_layer = dxf_layers_[0];
      break;
    case ByLayers:
      if(dxf_layers_.empty()) {
        dxf_layers_.push_back(std::make_shared<Layer>());
        dxf_layers_[0]->setName(layer_name);
        target_layer = dxf_layers_[0];
        target_layer->setColor(color);
      } else {
        for(int i = 0;i < dxf_layers_.size(); ++i) {
          target_layer = dxf_layers_[i];
          if(target_layer->name() == layer_name) {
            break;
          } else if(i == dxf_layers_.size()-1) {
            target_layer = std::make_shared<Layer>();
            target_layer->setName(layer_name);
            target_layer->setColor(color);
            dxf_layers_.push_back(target_layer);
          }
        }
      }
      break;
    case ByColors:
      layer_name = color.name();
      if(dxf_layers_.empty()) {
        dxf_layers_.push_back(std::make_shared<Layer>());
        dxf_layers_[0]->setName(layer_name);
        target_layer = dxf_layers_[0];
        target_layer->setColor(color);
      } else {
        for(int i = 0;i < dxf_layers_.size(); ++i) {
          target_layer = dxf_layers_[i];
          if(target_layer->name() == layer_name) {
            break;
          } else if(i == dxf_layers_.size()-1) {
            target_layer = std::make_shared<Layer>();
            target_layer->setName(layer_name);
            target_layer->setColor(color);
            dxf_layers_.push_back(target_layer);
          }
        }
      }
      break;
  }
  return target_layer;
}

void DXFReader::handleEntityLine(RS_Line* entity) {
  QString layer_name = entity->getLayer()->getName();
  QColor color = entity->getPen().getColor().toQColor();
  LayerPtr target_layer = findLayer(layer_name, color);
  
  QPainterPath working_path;
  working_path.moveTo(entity->getStartpoint().x, entity->getStartpoint().y);
  working_path.lineTo(entity->getEndpoint().x, entity->getEndpoint().y);
  ShapePtr new_shape = std::make_shared<PathShape>(working_path);
  target_layer->addShape(new_shape);
}

void DXFReader::handleEntityArc(RS_Arc* entity) {
  QString layer_name = entity->getLayer()->getName();
  QColor color = entity->getPen().getColor().toQColor();
  LayerPtr target_layer = findLayer(layer_name, color);

  RS_ArcData data = entity->getData();
  QPainterPath working_path;
  double angle1 = data.angle1;
  double angle2 = data.angle2;
  if(data.reversed) {
    double tmp = angle1;
    angle1 = angle2;
    angle2 = tmp;
  }
  working_path.moveTo(data.center.x + data.radius * cos(angle1), 
                      data.center.y + data.radius * sin(angle1));
  for(int i = 0 ; (i+2)<= 71; i+= 3) {
    if(angle1 < angle2) {
      working_path.cubicTo(data.center.x + data.radius * cos((double)i/71 * (angle2-angle1) + angle1),  
                           data.center.y + data.radius * sin((double)i/71 * (angle2-angle1) + angle1), 
                           data.center.x + data.radius * cos((double)(i+1)/71 * (angle2-angle1) + angle1),  
                           data.center.y + data.radius * sin((double)(i+1)/71 * (angle2-angle1) + angle1), 
                           data.center.x + data.radius * cos((double)(i+2)/71 * (angle2-angle1) + angle1),  
                           data.center.y + data.radius * sin((double)(i+2)/71 * (angle2-angle1) + angle1));
    } else {
      working_path.cubicTo(data.center.x + data.radius * cos((double)i/71 * (2*M_PI+angle2-angle1) + angle1), 
                           data.center.y + data.radius * sin((double)i/71 * (2*M_PI+angle2-angle1) + angle1), 
                           data.center.x + data.radius * cos((double)(i+1)/71 * (2*M_PI+angle2-angle1) + angle1), 
                           data.center.y + data.radius * sin((double)(i+1)/71 * (2*M_PI+angle2-angle1) + angle1), 
                           data.center.x + data.radius * cos((double)(i+2)/71 * (2*M_PI+angle2-angle1) + angle1), 
                           data.center.y + data.radius * sin((double)(i+2)/71 * (2*M_PI+angle2-angle1) + angle1));
    }
  }
  ShapePtr new_shape = std::make_shared<PathShape>(working_path);
  target_layer->addShape(new_shape);
}

void DXFReader::handleEntityCircle(RS_Circle* entity) {
  QString layer_name = entity->getLayer()->getName();
  QColor color = entity->getPen().getColor().toQColor();
  LayerPtr target_layer = findLayer(layer_name, color);

  RS_CircleData data = entity->getData();
  QPainterPath working_path;
  QPointF center(data.center.x, data.center.y);
  working_path.addEllipse(center, data.radius, data.radius);
  ShapePtr new_shape = std::make_shared<PathShape>(working_path);
  target_layer->addShape(new_shape);
}

void DXFReader::handleEntityEllipse(RS_Ellipse* entity) {
  QString layer_name = entity->getLayer()->getName();
  QColor color = entity->getPen().getColor().toQColor();
  LayerPtr target_layer = findLayer(layer_name, color);
  RS_Vector vect = entity->getMajorP();
  RS_EllipseData data = entity->getData();
  QPainterPath working_path;
  working_path.addEllipse(-1*entity->getMajorRadius(), -1*entity->getMinorRadius(), 
                          2*entity->getMajorRadius(), 2*entity->getMinorRadius());
  ShapePtr new_shape = std::make_shared<PathShape>(working_path);
  QTransform temp_trans = QTransform();
  temp_trans = temp_trans.rotateRadians(entity->getAngle());
  new_shape->applyTransform(temp_trans);
  temp_trans = QTransform();
  temp_trans = temp_trans.translate(data.center.x, data.center.y);
  new_shape->applyTransform(temp_trans);
  target_layer->addShape(new_shape);
}

void DXFReader::handleEntitySpline(RS_Spline* entity) {
  QString layer_name = entity->getLayer()->getName();
  QColor color = entity->getPen().getColor().toQColor();
  LayerPtr target_layer = findLayer(layer_name, color);

  RS_SplineData data = entity->getData();
  QPainterPath working_path;
  working_path.moveTo(data.controlPoints[0].x, data.controlPoints[0].y);
  unsigned int index = 1;
  for(; (index+2) < data.controlPoints.size(); index+=3) {
    working_path.cubicTo(data.controlPoints[index].x, data.controlPoints[index].y, 
                         data.controlPoints[index+1].x, data.controlPoints[index+1].y,
                         data.controlPoints[index+2].x, data.controlPoints[index+2].y);
  }
  if(index != data.controlPoints.size()) {
    working_path.cubicTo(data.controlPoints[data.controlPoints.size()-3].x, data.controlPoints[data.controlPoints.size()-3].y, 
                              data.controlPoints[data.controlPoints.size()-2].x, data.controlPoints[data.controlPoints.size()-2].y,
                              data.controlPoints[data.controlPoints.size()-1].x, data.controlPoints[data.controlPoints.size()-1].y);
  }
  ShapePtr new_shape = std::make_shared<PathShape>(working_path);
  target_layer->addShape(new_shape);
}

void DXFReader::handleEntityMText(RS_MText* entity) {
  std::cout << "getText = " << entity->getText().toStdString() << std::endl;
}

void DXFReader::handleEntityImage(RS_Image* entity) {
  QString layer_name = entity->getLayer()->getName();
  QColor color = entity->getPen().getColor().toQColor();
  LayerPtr target_layer = findLayer(layer_name, color);
  RS_ImageData data = entity->getData();
  QImage image;
  image = QImage(data.file);
  if (image.isNull()) {
    return;
  }
  std::shared_ptr<Shape> new_shape = std::make_shared<BitmapShape>(image);
  QSize org_size = image.size();
  QTransform temp_trans = QTransform();
  temp_trans = temp_trans.scale(data.uVector.magnitude(), 
                                data.vVector.magnitude());
  temp_trans.translate(data.insertionPoint.x, 
                       data.insertionPoint.y);
  new_shape->applyTransform(temp_trans);
  target_layer->addShape(new_shape);
}

void DXFReader::handleEntityContainer(RS_EntityContainer* container) {
  for(unsigned int i = 0; i < container->count(); ++i) {
      int entity_type = container->entityAt(i)->rtti();
      //still some type doesn't handle
      switch(entity_type) {
        case RS2::EntityLine:
          handleEntityLine((RS_Line*)container->entityAt(i));
          break;
        case RS2::EntityPolyline:
          handleEntityContainer((RS_EntityContainer*)container->entityAt(i));
          break;
        case RS2::EntityArc:
          handleEntityArc((RS_Arc*)container->entityAt(i));
          break;
        case RS2::EntityCircle:
          handleEntityCircle((RS_Circle*)container->entityAt(i));
          break;
        case RS2::EntityInsert:
          handleEntityContainer((RS_EntityContainer*)container->entityAt(i));
          break;
        case RS2::EntitySpline:
          handleEntitySpline((RS_Spline*)container->entityAt(i));
          break;
        case RS2::EntityEllipse:
          handleEntityEllipse((RS_Ellipse*)container->entityAt(i));
          break;
        // case RS2::EntityMText:
        //   handleEntityMText((RS_MText*)container->entityAt(i));
        //   break;
        // case RS2::EntityImage:
        //   handleEntityImage((RS_Image*)container->entityAt(i));
        //   break;
        default:
          qInfo() << "the entity type = " << entity_type << " need to handle";
          // std::cout << "the entity = " << container->entityAt(i) << std::endl;
          break;
      } 
  }
}

void DXFReader::openFile(const QString &file_name, QList<LayerPtr> &dxf_layers, ReadType read_type) {
  RS_Graphic* graphic = new RS_Graphic();
  graphic->open(file_name, RS2::FormatUnknown);
  read_type_ = read_type;

  handleEntityContainer((RS_EntityContainer*)graphic);
  QRectF rect = QRectF();
  for (auto &layer : dxf_layers_) {
    for (auto shape : layer->children()) {
      QTransform temp_trans = QTransform();
      temp_trans = temp_trans.scale(10, -10);
      shape->applyTransform(temp_trans);
      rect |= shape->boundingRect();
    }
    dxf_layers.push_back(layer);
  }
  for (auto &layer : dxf_layers_) {
    for (auto shape : layer->children()) {
      QTransform temp_trans = QTransform();
      temp_trans.translate(-1*rect.topLeft().x(), -1*rect.topLeft().y());
      shape->applyTransform(temp_trans);
    }
  }
}
