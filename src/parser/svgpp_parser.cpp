#include <parser/svgpp_impl.h>
#include <parser/svgpp_context.h>
#include <parser/svgpp_parser.h>

SVGPPParser::SVGPPParser(Scene &scene): scene_(scene) {
}

bool SVGPPParser::parse(QByteArray &data) {
    SVGPPContext context(scene_);
    return svgpp_parse2(data, context);
}
