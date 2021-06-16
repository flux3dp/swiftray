#include <undo.h>
#include <document.h>


AddLayerEvent::undo() {
  document.removeLayer(layer_);
}

AddLayerEvent::redo() {
  document.addLayer(layer_);
}

RemoveLayerEvent::undo() {
  document.addLayer(layer_);
}

RemoveLayerEvent::redo() {
  document.removeLayer(layer_);
}
