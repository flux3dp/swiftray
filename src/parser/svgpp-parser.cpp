#include <QDebug>
#include <parser/svgpp-common.h>
#include <parser/svgpp-parser.h>

#include <document.h>

namespace Parser {

bool SVGPPParser::parse(Document *doc, QByteArray &data, QList<LayerPtr> *svg_layers) {
  bool success = svgpp_parse(data);
  qInfo() << "Total layers" << svgpp_layers->size();
  for (auto &layer : *svgpp_layers) {
    doc->addLayer(layer);
    svg_layers->push_back(layer);
  }
  return success;
}

}