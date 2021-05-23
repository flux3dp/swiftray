#include "svgpp_impl.hpp"
#include "svgpp_context.hpp"
#include "svgpp_parser.hpp"

SVGPPParser::SVGPPParser() noexcept {
}

bool SVGPPParser::parse(QByteArray &data) {
    SVGPPContext context(&paths);
    return svgpp_parse(data, context);
}
