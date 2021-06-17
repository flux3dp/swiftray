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
  qInfo() << "Undo add shape" << (int) shape_->type();
  layer_->removeShape(shape_);
}

void AddShapeEvent::redo() {
  qInfo() << "Redo add shape" << (int) shape_->type();
  layer_->addShape(shape_);
}


void RemoveShapeEvent::undo() {
  qInfo() << "Undo remove shape" << (int) shape_->type();
  layer_->addShape(shape_);
}

void RemoveShapeEvent::redo() {
  qInfo() << "Undo add shape" << (int) shape_->type();
  layer_->removeShape(shape_);
}

SelectionEvent::SelectionEvent() {
  origin_selections_.clear();
  origin_selections_.append(VCanvas::document().selections());
}

void SelectionEvent::undo() {
  qInfo() << "Undo selection event" << origin_selections_.size();
  redo_selections_.clear();
  redo_selections_.append(VCanvas::document().selections());
  VCanvas::document().setSelections(origin_selections_);
}

void SelectionEvent::redo() {
  qInfo() << "Redo selection event" << redo_selections_.size();
  VCanvas::document().setSelections(redo_selections_);
}

