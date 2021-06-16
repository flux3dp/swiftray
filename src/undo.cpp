#include <undo.h>
#include <canvas/vcanvas.h>

void AddLayerEvent::undo() {
  VCanvas::document().removeLayer(layer_);
}

void AddLayerEvent::redo() {
  VCanvas::document().addLayer(layer_);
}

void RemoveLayerEvent::undo() {
  VCanvas::document().addLayer(layer_);
}

void RemoveLayerEvent::redo() {
  VCanvas::document().removeLayer(layer_);
}

void AddShapeEvent::undo() {
  qInfo() << "Undoing add shape" << shape_.get() << "with layer" << layer_->name();
  layer_->removeShape(shape_);
}

void AddShapeEvent::redo() {
  qInfo() << "Redoing add shape";
  layer_->addShape(shape_);
}


void RemoveShapeEvent::undo() {
  layer_->addShape(shape_);
}

void RemoveShapeEvent::redo() {
  layer_->removeShape(shape_);
}