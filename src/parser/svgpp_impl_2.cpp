#include <QByteArray>
#include <QDebug>
#include <iostream>
#include <parser/svgpp_common.h>

#include <parser/stylable_context.h>
#include <parser/svgpp_context.h>
#include <parser/svgpp_impl.h>

using namespace svgpp;

class ObjectContext {
public:
  void set(tag::attribute::width, double val){};
  void set(tag::attribute::height, double val){};
  void set(tag::attribute::preserveAspectRatio, bool, tag::value::none){};

  void set(tag::attribute::color, color_t){};
  void set(tag::attribute::marker, tag::value::none){};

  void set(tag::attribute::marker_end, tag::value::none){};
  void set(tag::attribute::marker_mid, tag::value::none){};
  void set(tag::attribute::marker_start, tag::value::none){};

  void set(svgpp::tag::attribute::data_strength, double val) {
    qInfo() << "Data-Strength" << val;
  }

  void set(svgpp::tag::attribute::data_speed, double val) {
    qInfo() << "Data-Speed" << val;
  }

  void set(svgpp::tag::attribute::data_repeat, int val) {}
  void set(svgpp::tag::attribute::data_height, double val) {}
  void set(svgpp::tag::attribute::data_diode, int val) {}
  void set(svgpp::tag::attribute::data_zstep, double val) {}

  template <class IRI>
  void set(svgpp::tag::attribute::data_config_name, tag::iri_fragment,
           IRI const &fragment) {
    qInfo() << "xlink::href" << fragment;
  }

  void set(svgpp::tag::attribute::data_config_name, RangedChar fragment) {
    qInfo() << "xlink::href"
            << QString::fromStdString(
                   std::string(fragment.begin(), fragment.end()));
  }

  template <class IRI>
  void set(svgpp::tag::attribute::data_original_layer, tag::iri_fragment,
           IRI const &fragment) {
    qInfo() << "xlink::href" << fragment;
  }

  void set(svgpp::tag::attribute::data_original_layer, RangedChar fragment) {
    qInfo() << "xlink::href"
            << QString::fromStdString(
                   std::string(fragment.begin(), fragment.end()));
  }

  template <class IRI>
  void set(tag::attribute::xlink::href, tag::iri_fragment,
           IRI const &fragment) {
    qInfo() << "xlink::href" << fragment;
  }

  void set(tag::attribute::xlink::href, RangedChar fragment) {
    qInfo() << "xlink::href"
            << QString::fromStdString(
                   std::string(fragment.begin(), fragment.end()));
  }

  template <typename MaskType> void set(tag::attribute::mask, MaskType){};
  template <typename Range>
  void set(tag::attribute::mask, tag::iri_fragment, Range){};

  template <typename Range>
  void set(tag::attribute::marker, tag::iri_fragment, Range){};
  template <typename Range> void set(tag::attribute::marker, Range){};

  template <typename Range>
  void set(tag::attribute::marker_start, tag::iri_fragment, Range){};
  template <typename Range> void set(tag::attribute::marker_start, Range){};

  template <typename Range>
  void set(tag::attribute::marker_mid, tag::iri_fragment, Range){};
  template <typename Range> void set(tag::attribute::marker_mid, Range){};

  template <typename Range>
  void set(tag::attribute::marker_end, tag::iri_fragment, Range){};
  template <typename Range> void set(tag::attribute::marker_end, Range){};

  template <typename MinMax, typename SliceMeet>
  void set(tag::attribute::preserveAspectRatio, bool, MinMax, SliceMeet){};
  template <typename MaskType> void set(tag::attribute::clip_path, MaskType){};
  template <typename Range>
  void set(tag::attribute::clip_path, tag::iri_fragment, Range){};

private:
  double empty;
};

class BaseContext : public StylableContext, public ObjectContext {
public:
  BaseContext(double resolutionDPI) {
    qInfo() << "Enter base";
    length_factory_.set_absolute_units_coefficient(resolutionDPI,
                                                   tag::length_units::in());
  }

  using ObjectContext::set;
  using StylableContext::set;

  // Called by Context Factory
  void on_enter_element(tag::element::path) { qInfo() << "Enter path"; }

  void on_enter_element(tag::element::rect) { qInfo() << "Enter rect"; }

  void on_enter_element(tag::element::text) { qInfo() << "Enter text"; }

  void on_enter_element(tag::element::image) { qInfo() << "Enter image"; }

  // Called by Context Factory
  void on_exit_element() { qInfo() << "Exit ele"; }

  // Transform Events Policy
  void transform_matrix(const boost::array<double, 6> &matrix) {}

  // Viewport Events Policy
  void set_viewport(double viewport_x, double viewport_y, double viewport_width,
                    double viewport_height) {
    length_factory_.set_viewport_size(viewport_width, viewport_height);
  }

  void set_viewbox_size(double viewbox_width, double viewbox_height) {
    length_factory_.set_viewport_size(viewbox_width, viewbox_height);
  }

  void disable_rendering() {}

  static bool unknown_attribute_error(std::string name) {
    qInfo() << "Unknown attribute" << QString::fromStdString(name);
    return true;
  }

  // If textcontext doesn't work
  template <class T> void set_text(T const &text) {
    qInfo() << "Set text" << text;
  }

  template <class T> void set(tag::attribute::x, T const &range) {}

  template <class T> void set(tag::attribute::y, T const &range) {}

  // Length Policy interface
  typedef factory::length::unitless<> length_factory_type;

  length_factory_type const &length_factory() const { return length_factory_; }

private:
  length_factory_type length_factory_;
};

class ShapeContext : public BaseContext {
public:
  ShapeContext(BaseContext const &parent) : BaseContext(parent) {
    qInfo() << "Enter shape";
  }

  using BaseContext::set;
  using StylableContext::set;
  // Path Events Policy methods
  void path_move_to(double x, double y, tag::coordinate::absolute) {
    std::cout << "Move to " << x << "," << y << std::endl;
  }

  void path_line_to(double x, double y, tag::coordinate::absolute) {
    // std::cout << "Line to " << x << "," << y << std::endl;
  }

  void path_cubic_bezier_to(double x1, double y1, double x2, double y2,
                            double x, double y, tag::coordinate::absolute) {}

  void path_quadratic_bezier_to(double x1, double y1, double x, double y,
                                tag::coordinate::absolute) {}

  void path_elliptical_arc_to(double rx, double ry, double x_axis_rotation,
                              bool large_arc_flag, bool sweep_flag, double x,
                              double y, tag::coordinate::absolute) {}

  void path_close_subpath() {}

  void path_exit() {}

  // Marker Events Policy method
  void marker(marker_vertex v, double x, double y, double directionality,
              unsigned marker_index) {
    if (marker_index >= markers_.size())
      markers_.resize(marker_index + 1);
    MarkerPos &m = markers_[marker_index];
    m.v = v;
    m.x = x;
    m.y = y;
    m.directionality = directionality;
  }

private:
  struct MarkerPos {
    marker_vertex v;
    double x, y, directionality;
  };

  typedef std::vector<MarkerPos> Markers;
  Markers markers_;
};

class UseContext : public BaseContext {
public:
  UseContext(BaseContext const &parent) : BaseContext(parent) {
    qInfo() << "Enter use";
  }

  boost::optional<double> const &width() const { return width_; }
  boost::optional<double> const &height() const { return height_; }

  using BaseContext::set;

  template <class IRI>
  void set(tag::attribute::xlink::href, tag::iri_fragment,
           IRI const &fragment) {
    fragment_id_.assign(boost::begin(fragment), boost::end(fragment));
  }

  template <class IRI>
  void set(tag::attribute::xlink::href, IRI const &fragment) {
    std::cerr << "External references aren't supported\n";
  }

  void set(tag::attribute::x, double val) { x_ = val; }

  void set(tag::attribute::y, double val) { y_ = val; }

  void set(tag::attribute::width, double val) { width_ = val; }

  void set(tag::attribute::height, double val) { height_ = val; }

  void on_exit_element();

private:
  std::string fragment_id_;
  double x_, y_;
  boost::optional<double> width_, height_;
};

class ImageContext : public BaseContext {
public:
  ImageContext(BaseContext const &parent) : BaseContext(parent) {
    qInfo() << "Enter image";
  }

  boost::optional<double> const &width() const { return width_; }
  boost::optional<double> const &height() const { return height_; }

  using BaseContext::set;

  template <class IRI>
  void set(tag::attribute::xlink::href, tag::iri_fragment,
           IRI const &fragment) {
    qInfo() << "xlink::href" << fragment;
  }

  void set(tag::attribute::xlink::href, RangedChar fragment) {
    qInfo() << "xlink::href"
            << QString::fromStdString(
                   std::string(fragment.begin(), fragment.end()));
  }

  void set(tag::attribute::x, double val) { x_ = val; }

  void set(tag::attribute::y, double val) { y_ = val; }

  void set(tag::attribute::width, double val) { width_ = val; }

  void set(tag::attribute::height, double val) { height_ = val; }

  void on_exit_element();

private:
  std::string fragment_id_;
  double x_, y_;
  boost::optional<double> width_, height_;
};

class ReferencedSymbolOrSvgContext : public BaseContext {
public:
  ReferencedSymbolOrSvgContext(UseContext &referencing)
      : BaseContext(referencing), referencing_(referencing) {}

  // Viewport Events Policy
  void get_reference_viewport_size(double &width, double &height) {
    if (referencing_.width())
      width = *referencing_.width();
    if (referencing_.height())
      height = *referencing_.height();
  }

private:
  UseContext &referencing_;
};

class TextContext : public BaseContext {
public:
  TextContext(BaseContext const &parent) : BaseContext(parent), x_(0), y_(0) {
    qInfo() << "Enter text";
  }

  template <class Range> void set_text(Range const &text) {
    text_content_.append(boost::begin(text), boost::end(text));
  }

  using ObjectContext::set;
  using StylableContext::set;

  template <class Range> void set(tag::attribute::x, Range const &range) {
    for (typename boost::range_iterator<Range>::type it = boost::begin(range),
                                                     end = boost::end(range);
         it != end; ++it)
      std::cout << *it;
  }

  template <class Range> void set(tag::attribute::y, Range const &range) {
    for (typename boost::range_iterator<Range>::type it = boost::begin(range),
                                                     end = boost::end(range);
         it != end; ++it)
      std::cout << *it;
  }

private:
  std::string text_content_;
  double x_, y_;
};

class GroupContext : public ShapeContext {
public:
  GroupContext(ShapeContext const &parent) : ShapeContext(parent) {
    qInfo() << "Enter text";
  }
  using ObjectContext::set;
  using ShapeContext::set;
  using StylableContext::set;
};

struct ChildContextFactories {
  template <class ParentContext, class ElementTag, class Enable = void>
  struct apply {
    // Default definition handles "svg" and "g" elements
    typedef factory::context::on_stack<BaseContext> type;
  };
};

/*template<>
struct ChildContextFactories::apply<BaseContext, tag::element::g, void> {
  typedef factory::context::on_stack<GroupContext> type;
};*/

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
struct ChildContextFactories::apply<BaseContext, tag::element::use_> {
  typedef factory::context::on_stack<UseContext> type;
};

template <>
struct ChildContextFactories::apply<BaseContext, tag::element::image> {
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
          boost::mpl::pair<svgpp::tag::element::g,
                           svgpp::tag::attribute::data_original_layer>> {};

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
    transform_events_policy<policy::transform_events::forward_to_method<
        BaseContext>> // Same as default, but less instantiations
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

bool svgpp_parse2(QByteArray &data, SVGPPContext &context) {
  try {
    // TODO: Support UTF8
    xmlDoc *doc = xmlParseDoc((const unsigned char *)data.constData());
    xmlNode *root = xmlDocGetRootElement(doc);

    if (root) {
      qInfo() << "SVG Element " << root;
      static const double ResolutionDPI = 90;
      BaseContext context(ResolutionDPI);
      document_traversal_t::load_document(root, context);
      qInfo() << "Loaded SVG";
      return true;
    }
  } catch (std::exception const &e) {
    qWarning() << "Error loading SVG: " << e.what();
  }

  return false;
}
