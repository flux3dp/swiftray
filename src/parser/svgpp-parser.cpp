#include <QDebug>
#include <parser/svgpp-common.h>
#include <parser/svgpp-parser.h>

#include <document.h>

bool SVGPPParser::parse(Document *doc, QByteArray &data) {
  bool success = svgpp_parse(data);
  qInfo() << "Total layers" << svgpp_layers->size();
  for (auto &layer : *svgpp_layers) {
    doc->addLayer(layer);
  }
  return success;
}
