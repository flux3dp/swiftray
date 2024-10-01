#pragma once

#include <QString>

namespace Parser {
    class PDF2SVG {
    public:
        PDF2SVG();
        ~PDF2SVG();
        void convertPDFFile(QString target_pdf, QString svg_filename);
        void removeSVGFile(QString svg_filename);
    };
}