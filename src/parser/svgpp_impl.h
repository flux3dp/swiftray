#include <QByteArray>
#include <parser/svgpp_context.h>

#ifndef SVGPPIMPL_H
#define SVGPPIMPL_H
bool svgpp_parse(QByteArray &data, SVGPPContext &context);
bool svgpp_parse2(QByteArray &data, SVGPPContext &context);
#endif