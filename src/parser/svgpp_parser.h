#include <QPainterPath>
#include <document.h>

#ifndef SVGPP_PARSER_H
#define SVGPP_PARSER_H

class SVGPPParser {
public:
  SVGPPParser(Document &scene);

  Document &scene_;

  bool parse(QByteArray &data);
};

#endif // SVGPP_PARSER_H
