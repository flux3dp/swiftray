#include "svgpp_impl.hpp"
#include "svgpp_context.hpp"
#include "svgpp_parser.hpp"

SVGPPParser::SVGPPParser(QList<QPainterPath> *pathsRef) {
    this->pathsRef = pathsRef;
}

bool SVGPPParser::parse(QByteArray &data) {
    SVGPPContext context(pathsRef);
    return svgpp_parse(data, context);
}
