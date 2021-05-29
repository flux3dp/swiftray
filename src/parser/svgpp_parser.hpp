#include <QPainterPath>
#include <canvas/canvas_data.hpp>

#ifndef SVGPP_PARSER_H
#define SVGPP_PARSER_H

class SVGPPParser {
    public:
        SVGPPParser(CanvasData &canvas);
        CanvasData &canvas_;
        bool parse(QByteArray &data);
};

#endif // SVGPP_PARSER_H
