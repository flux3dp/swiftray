#include "svgpp_impl.hpp"
#include "svgpp_context.hpp"
#include "svgpp_parser.hpp"

SVGPPParser::SVGPPParser(QList<Shape> *shapes) {
    this->shapes = shapes;
}

bool SVGPPParser::parse(QByteArray &data) {
    SVGPPContext context(shapes);
    return svgpp_parse(data, context);
}
