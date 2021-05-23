#include <QPainterPath>
#include <shape/shape.hpp>

#ifndef SVGPP_PARSER_H
#define SVGPP_PARSER_H

class SVGPPParser {
    public:
        SVGPPParser() noexcept;
        QList<QPainterPath> paths;
        QList<Shape> shapes;
        bool parse(QByteArray &data);
};

#endif // SVGPP_PARSER_H
