#include "svgpp_impl.hpp"
#include "svgpp_context.hpp"
#include "svgpp_parser.hpp"

SVGPPParser::SVGPPParser(Scene &canvas): scene_ { canvas } {
}

bool SVGPPParser::parse(QByteArray &data) {
    SVGPPContext context(scene_);
    return svgpp_parse(data, context);
}
