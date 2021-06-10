#include <QByteArray>
#include <QMap>
#include <QDebug>
#include <iostream>
#include <parser/svgpp_common.h>

#include <parser/contexts/base_context.h>
#include <parser/contexts/group_context.h>
#include <parser/contexts/image_context.h>
#include <parser/contexts/reference_context.h>
#include <parser/contexts/shape_context.h>
#include <parser/contexts/text_context.h>
#include <parser/contexts/use_context.h>

#include <canvas/layer.h>

using namespace svgpp;

struct ChildContextFactories {
  template <class ParentContext, class ElementTag, class Enable = void>
  struct apply {
    // Default definition handles "svg" and "g" elements
    typedef factory::context::on_stack<BaseContext> type;
  };
};

template <>
struct ChildContextFactories::apply<BaseContext, tag::element::g, void> {
  typedef factory::context::on_stack<GroupContext> type;
};

// This specialization handles all shape elements (elements from
// traits::shape_elements sequence)
template <class ElementTag>
struct ChildContextFactories::apply<
    BaseContext, ElementTag,
    typename boost::enable_if<
        boost::mpl::has_key<traits::shape_elements, ElementTag>>::type> {
  typedef factory::context::on_stack<ShapeContext> type;
};

template <>
struct ChildContextFactories::apply<BaseContext, tag::element::use_, void> {
  typedef factory::context::on_stack<UseContext> type;
};

template <>
struct ChildContextFactories::apply<BaseContext, tag::element::image, void> {
  typedef factory::context::on_stack<ImageContext> type;
};

// Elements referenced by 'use' element
template <>
struct ChildContextFactories::apply<UseContext, tag::element::svg, void> {
  typedef factory::context::on_stack<ReferencedSymbolOrSvgContext> type;
};

template <>
struct ChildContextFactories::apply<UseContext, tag::element::symbol, void> {
  typedef factory::context::on_stack<ReferencedSymbolOrSvgContext> type;
};

template <class ElementTag>
struct ChildContextFactories::apply<UseContext, ElementTag, void>
    : ChildContextFactories::apply<BaseContext, ElementTag> {};

template <class ElementTag>
struct ChildContextFactories::apply<ReferencedSymbolOrSvgContext, ElementTag,
                                    void>
    : ChildContextFactories::apply<BaseContext, ElementTag> {};

template <>
struct ChildContextFactories::apply<BaseContext, tag::element::text> {
  typedef factory::context::on_stack<TextContext> type;
};

template <>
struct ChildContextFactories::apply<BaseContext, tag::element::tspan> {
  typedef factory::context::on_stack<TextContext> type;
};

template <>
struct ChildContextFactories::apply<
    TextContext,
    tag::element::tspan> { // You need to redirect tspan under TextContext
                           // because it's a sub textcontext item
  typedef factory::context::on_stack<TextContext> type;
};

template <class ElementTag>
struct ChildContextFactories::apply<GroupContext, ElementTag>
    : ChildContextFactories::apply<BaseContext, ElementTag> {};

struct AttributeTraversal : policy::attribute_traversal::default_policy {
  typedef boost::mpl::if_<
      // If element is 'svg' or 'symbol'...
      boost::mpl::has_key<
          boost::mpl::set<tag::element::svg, tag::element::symbol>,
          boost::mpl::_1>,
      boost::mpl::vector<
          // ... load viewport-related attributes first ...
          tag::attribute::x, tag::attribute::y, tag::attribute::width,
          tag::attribute::height, tag::attribute::viewBox,
          tag::attribute::preserveAspectRatio,
          // ... notify library, that all viewport attributes that are present
          // was loaded. It will result in call to BaseContext::set_viewport and
          // BaseContext::set_viewbox_size
          notify_context<tag::event::after_viewport_attributes>>::type,
      boost::mpl::empty_sequence>
      get_priority_attributes_by_element;
};

struct processed_elements_t
    : boost::mpl::set<
          // SVG Structural Elements
          tag::element::svg, tag::element::g, tag::element::use_,
          // SVG Shape Elements
          tag::element::circle, tag::element::ellipse, tag::element::line,
          tag::element::path, tag::element::polygon, tag::element::polyline,
          tag::element::rect,
          // Text Element
          tag::element::text, tag::element::tspan,
          // Image Element
          tag::element::image> {};

// Joining some sequences from traits namespace with chosen attributes
struct processed_attributes_t
    : boost::mpl::set50<
          svgpp::tag::attribute::transform, svgpp::tag::attribute::stroke,
          svgpp::tag::attribute::stroke_width,
          boost::mpl::pair<svgpp::tag::element::use_,
                           svgpp::tag::attribute::xlink::href>,
          // traits::shapes_attributes_by_element,
          boost::mpl::pair<tag::element::path, tag::attribute::d>,
          boost::mpl::pair<tag::element::rect, tag::attribute::x>,
          boost::mpl::pair<tag::element::rect, tag::attribute::y>,
          boost::mpl::pair<tag::element::rect, tag::attribute::width>,
          boost::mpl::pair<tag::element::rect, tag::attribute::height>,
          boost::mpl::pair<tag::element::rect, tag::attribute::rx>,
          boost::mpl::pair<tag::element::rect, tag::attribute::ry>,
          boost::mpl::pair<tag::element::circle, tag::attribute::cx>,
          boost::mpl::pair<tag::element::circle, tag::attribute::cy>,
          boost::mpl::pair<tag::element::circle, tag::attribute::r>,
          boost::mpl::pair<tag::element::ellipse, tag::attribute::cx>,
          boost::mpl::pair<tag::element::ellipse, tag::attribute::cy>,
          boost::mpl::pair<tag::element::ellipse, tag::attribute::rx>,
          boost::mpl::pair<tag::element::ellipse, tag::attribute::ry>,
          boost::mpl::pair<tag::element::line, tag::attribute::x1>,
          boost::mpl::pair<tag::element::line, tag::attribute::y1>,
          boost::mpl::pair<tag::element::line, tag::attribute::x2>,
          boost::mpl::pair<tag::element::line, tag::attribute::y2>,
          boost::mpl::pair<tag::element::polyline, tag::attribute::points>,
          boost::mpl::pair<tag::element::polygon, tag::attribute::points>,
          // Marker attributes
          tag::attribute::marker_start, tag::attribute::marker_mid,
          tag::attribute::marker_end, tag::attribute::marker,
          tag::attribute::markerUnits, tag::attribute::markerWidth,
          // Shape attributes
          tag::attribute::clip_path, tag::attribute::color,
          tag::attribute::fill, tag::attribute::mask,
          // traits::viewport_attributes
          tag::attribute::x, tag::attribute::y, tag::attribute::width,
          tag::attribute::height, tag::attribute::viewBox,
          boost::mpl::pair<svgpp::tag::element::svg,
                           tag::attribute::preserveAspectRatio>,
          boost::mpl::pair<svgpp::tag::element::use_,
                           tag::attribute::preserveAspectRatio>,
          // image
          boost::mpl::pair<svgpp::tag::element::image,
                           svgpp::tag::attribute::xlink::href>,
          // group data
          boost::mpl::pair<svgpp::tag::element::g,
                           svgpp::tag::attribute::data_strength>,
          boost::mpl::pair<svgpp::tag::element::g,
                           svgpp::tag::attribute::data_speed>,
          boost::mpl::pair<svgpp::tag::element::g,
                           svgpp::tag::attribute::data_repeat>,
          boost::mpl::pair<svgpp::tag::element::g,
                           svgpp::tag::attribute::data_height>,
          boost::mpl::pair<svgpp::tag::element::g,
                           svgpp::tag::attribute::data_diode>,
          boost::mpl::pair<svgpp::tag::element::g,
                           svgpp::tag::attribute::data_zstep>,
          boost::mpl::pair<svgpp::tag::element::g,
                           svgpp::tag::attribute::data_config_name>,
          svgpp::tag::attribute::data_original_layer> {};

struct ignored_elements_t : boost::mpl::set<tag::element::animateMotion> {};

typedef document_traversal<
    processed_elements<processed_elements_t>,
    processed_attributes<processed_attributes_t>,
    viewport_policy<policy::viewport::as_transform>,
    context_factories<ChildContextFactories>,
    markers_policy<policy::markers::calculate_always>,
    color_factory<ColorFactory>,
    length_policy<policy::length::forward_to_method<BaseContext>>,
    attribute_traversal_policy<AttributeTraversal>,
    error_policy<policy::error::default_policy<BaseContext>>,
    transform_events_policy<policy::transform_events::forward_to_method<BaseContext>> // Same as default, but less instantiations
    >
    document_traversal_t;

xmlNode *FindCurrentDocumentElementById(std::string const &) { return NULL; }

struct processed_elements_with_symbol_t
    : boost::mpl::insert<processed_elements_t::type,
                         tag::element::symbol>::type {};

void UseContext::on_exit_element() {
  if (xmlNode *element = FindCurrentDocumentElementById(fragment_id_)) {
    // TODO: Check for cyclic references
    // TODO: Apply translate transform (x_, y_)
    document_traversal_t::load_referenced_element<
        referencing_element<tag::element::use_>,
        expected_elements<traits::reusable_elements>,
        processed_elements<processed_elements_with_symbol_t>>::load(element,
                                                                    *this);
  } else
    std::cerr << "Element referenced by 'use' not found\n";
}

void ImageContext::on_exit_element() {}

void loadSvg(xmlNode *xml_root_element) {
  static const double ResolutionDPI = 90;
  BaseContext context(ResolutionDPI);
  document_traversal_t::load_document(xml_root_element, context);
}

QList<Layer> *svgpp_layers = new QList<Layer>();
QMap<QString, Layer*> *svgpp_layer_map = new QMap<QString, Layer*>();

bool svgpp_parse(QByteArray &data) {
  try {
    // TODO: Support UTF8
    xmlDoc *doc = xmlParseDoc((const unsigned char *)data.constData());
    xmlNode *root = xmlDocGetRootElement(doc);

    if (root) {
      qInfo() << "SVG Element " << root;
      static const double ResolutionDPI = 90;
      BaseContext context(ResolutionDPI);
      svgpp_layers->clear();
      svgpp_layer_map->clear();
      document_traversal_t::load_document(root, context);
      qInfo() << "Loaded SVG";
      return true;
    }
  } catch (std::exception const &e) {
    qWarning() << "Error loading SVG: " << e.what();
  }

  return false;
}

void svgpp_add_shape(ShapePtr shape, QString layer_name) {
  if (svgpp_layer_map->contains(layer_name)) {
    auto layer_iter = svgpp_layer_map->find(layer_name);
    if (layer_iter != svgpp_layer_map->end()) {
      (*layer_iter)->addShape(shape);
    }
  } else {
    svgpp_layers->push_back(Layer());
    svgpp_layers->last().name = layer_name;
    svgpp_layers->last().setColor(QColor(layer_name));
    svgpp_layers->last().addShape(shape);
    svgpp_layer_map->insert(layer_name, &(svgpp_layers->last()));
  }
}