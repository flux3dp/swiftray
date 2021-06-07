#include <parser/svgpp_common.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <svgpp/policy/xml/libxml2.hpp>
#include <svgpp/svgpp.hpp>
#include <svgpp/parser/external_function/parse_all_impl.hpp>
#include <svgpp/factory/color.hpp>
#include <parser/svgpp_color_factory.h>
#include <parser/svgpp_impl.h>
#include <parser/svgpp_context.h>

using namespace svgpp;

/* Building external parsing function (meant to accelerate compiling process) */
SVGPP_PARSE_PATH_DATA_IMPL(const char *, double)
SVGPP_PARSE_TRANSFORM_IMPL(const char *, double)
SVGPP_PARSE_PAINT_IMPL(const char *, ColorFactory, factory::icc_color::default_factory)
SVGPP_PARSE_COLOR_IMPL(const char *, ColorFactory, factory::icc_color::default_factory)
SVGPP_PARSE_PRESERVE_ASPECT_RATIO_IMPL(const char *)
SVGPP_PARSE_MISC_IMPL(const char *, double)

#include <boost/mpl/set.hpp>
#include <QByteArray>
#include <QDebug>

typedef
boost::mpl::set <
tag::element::svg,
    tag::element::g,
    tag::element::circle,
    tag::element::ellipse,
    tag::element::line,
    tag::element::path,
    tag::element::polygon,
    tag::element::polyline,
    tag::element::rect
    >::type processed_elements1_t;

bool svgpp_parse(QByteArray &data, SVGPPContext &context) {
    try {
        // TODO: Support UTF8
        xmlDoc *doc = xmlParseDoc((const unsigned char *)data.constData());
        xmlNode *root = xmlDocGetRootElement(doc);

        if (root) {
            qInfo() << "SVG Element " << root;
            document_traversal < processed_elements<processed_elements1_t>,
                               processed_attributes<traits::shapes_attributes_by_element>,
                               transform_events_policy<policy::transform_events::forward_to_method<SVGPPContext>>,
                               svgpp::error_policy<svgpp::policy::error::default_policy<SVGPPContext>>
                               >::load_document(root, context);
            qInfo() << "Loaded SVG";
            return true;
        }
    } catch (std::exception const &e) {
        qWarning() << "Error loading SVG: " << e.what();
    }

    return false;
}
