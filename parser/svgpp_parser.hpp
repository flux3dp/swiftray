#include <QPainterPath>
#include <shape/shape.hpp>
#include <container/shape_collection.h>

#ifndef SVGPP_PARSER_H
#define SVGPP_PARSER_H

class SVGPPParser {
    public:
        SVGPPParser(ShapeCollection &shapes);
        ShapeCollection &shapes_;
        bool parse(QByteArray &data);
};

#endif // SVGPP_PARSER_H
