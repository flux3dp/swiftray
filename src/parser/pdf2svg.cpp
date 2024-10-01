#include "pdf2svg.h"
#include <glib.h>
#include <glib/poppler.h>
#include <glib/poppler-document.h>
#include <glib/poppler-page.h>
#include <cairo.h>
#include <cairo-svg.h>
#include <QFile>

namespace Parser {

PDF2SVG::PDF2SVG()
{
}

PDF2SVG::~PDF2SVG()
{
}

void PDF2SVG::convertPDFFile(QString target_pdf, QString svg_filename)
{
    // Poppler stuff
	PopplerDocument *pdffile;
	PopplerPage *page;
    g_type_init ();
    gchar *filename_uri = g_filename_to_uri(target_pdf.toStdString().data(), NULL, NULL);
    // Open the PDF file
	pdffile = poppler_document_new_from_file(filename_uri, NULL, NULL);
	g_free(filename_uri);

    page = poppler_document_get_page(pdffile, 0);

    // Start converting the PDF file

	const char* svg_filename_str = svg_filename.toStdString().data();

    // Poppler stuff
    double width, height;

    // Cairo stuff
    cairo_surface_t *surface;
    cairo_t *drawcontext;

    if (page == NULL) {
        fprintf(stderr, "Page does not exist\n");
        return;
    }
    poppler_page_get_size (page, &width, &height);

    // Open the SVG file
    surface = cairo_svg_surface_create(svg_filename_str, width, height);
    drawcontext = cairo_create(surface);

    // Render the PDF file into the SVG file
    poppler_page_render_for_printing(page, drawcontext);
    cairo_show_page(drawcontext);

    // Close the SVG file
    cairo_destroy(drawcontext);
    cairo_surface_destroy(surface);

    // Close the PDF file
    g_object_unref(page);
}

void PDF2SVG::removeSVGFile(QString svg_filename) {
    QFile::remove(svg_filename);
}
}