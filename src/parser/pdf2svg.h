#pragma once

#include <glib.h>
#include <glib/poppler.h>
#include <glib/poppler-document.h>
#include <glib/poppler-page.h>
#include <cairo.h>
#include <cairo-svg.h>
#include <QString>

namespace Parser {
    class PDF2SVG {
    public:
        PDF2SVG();
        ~PDF2SVG();
        void convertPDFFile(QString target_pdf, QString svg_filename);
        void removeSVGFile(QString svg_filename);
    
    private:
        int convertPage(PopplerPage *page, const char* svgFilename);
    };

}