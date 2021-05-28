#include "svgpp_impl.hpp"
#include "svgpp_context.hpp"
#include "svgpp_parser.hpp"

SVGPPParser::SVGPPParser(QList<ShapePtr> &shapes): shapes_ { shapes } {
}

bool SVGPPParser::parse(QByteArray &data) {
    SVGPPContext context(shapes_);
    return svgpp_parse(data, context);
}
