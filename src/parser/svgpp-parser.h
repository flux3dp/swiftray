#ifndef SVGPP_PARSER_H
#define SVGPP_PARSER_H

#include <QPainterPath>

// This class acts like a bridge from main program to svgpp,
// to make the svgpp compiling dependency stops at "svgpp-parser.cpp"
// so the compiling process will be faster

class Document;

namespace Parser {

class SVGPPParser {
public:
  SVGPPParser() {};

  bool parse(Document *doc, QByteArray &data);
};

}

#endif // SVGPP_PARSER_H
