#include <parser/svgpp_common.h>
#include <parser/svgpp_parser.h>

SVGPPParser::SVGPPParser(Scene &scene) : scene_(scene) {
}

bool SVGPPParser::parse(QByteArray &data) {
  bool success = svgpp_parse(data);
  for (auto &layer : *svgpp_layers) {
    scene_.addLayer(layer);
  }
  scene_.emitAllChanges();
  return success;
}
