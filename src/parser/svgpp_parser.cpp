#include "svgpp_impl.hpp"
#include "svgpp_context.hpp"
#include "svgpp_parser.hpp"

SVGPPParser::SVGPPParser(CanvasData &canvas): canvas_ { canvas } {
}

bool SVGPPParser::parse(QByteArray &data) {
    SVGPPContext context(canvas_);
    return svgpp_parse(data, context);
}
