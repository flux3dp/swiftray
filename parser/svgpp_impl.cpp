#include <libxml/tree.h>
#include <libxml/parser.h>
#include <svgpp/policy/xml/libxml2.hpp>
#include <svgpp/svgpp.hpp>
#include <svgpp/parser/external_function/parse_all_impl.hpp>
#include <svgpp/factory/color.hpp>
#include "svgpp_common.hpp"
#include "svgpp_color_factory.hpp"
#include "svgpp_impl.hpp"

using namespace svgpp;

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
    >::type processed_elements_t;

bool svgpp_parse(QByteArray &data, VContext &context) {
    try {
        // TODO: Support UTF8
        xmlDoc *doc = xmlParseDoc((const unsigned char *)data.constData());
        xmlNode *root = xmlDocGetRootElement(doc);

        if (root) {
            qInfo() << "SVG Element " << root;
            context.clear();
            document_traversal < processed_elements<processed_elements_t>,
                               processed_attributes<traits::shapes_attributes_by_element>,
                               transform_events_policy<policy::transform_events::forward_to_method<VContext>>,
                               svgpp::error_policy<svgpp::policy::error::default_policy<VContext>>
                               >::load_document(root, context);
            qInfo() << "Loaded SVG " << context.getPathCount();
            return true;
        }
    } catch (std::exception const &e) {
        qWarning() << "Error loading SVG: " << e.what();
    }

    return false;
}
