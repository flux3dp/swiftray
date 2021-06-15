#include <QDebug>
#include <parser/svgpp_common.h>
#include <parser/svgpp_parser.h>

SVGPPParser::SVGPPParser(Document &scene) : scene_(scene) {
}

bool SVGPPParser::parse(QByteArray &data) {
  bool success = svgpp_parse(data);
  qInfo() << "Total layers" << svgpp_layers->size();
  for (auto &layer : *svgpp_layers) {
    scene_.addLayer(layer);
  }
  scene_.emitAllChanges();
  return success;
}
