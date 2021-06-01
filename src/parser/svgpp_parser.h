#include <QPainterPath>
#include <canvas/scene.h>

#ifndef SVGPP_PARSER_H
#define SVGPP_PARSER_H

class SVGPPParser {
    public:
        SVGPPParser(Scene &scene);
        Scene &scene_;
        bool parse(QByteArray &data);
};

#endif // SVGPP_PARSER_H
