#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_SET_SIZE 50
#define RENDERER_SKIA
#include "common.hpp"

#include <svgpp/document_traversal.hpp>
#include <svgpp/utility/gil/mask.hpp>

#include <boost/bind.hpp>
#include <boost/gil/algorithm.hpp>
#include <boost/gil/image_view.hpp>
#include <boost/gil/image_view_factory.hpp>
#include <boost/gil/typedefs.hpp>
#include <boost/mpl/set.hpp>
#include <boost/mpl/transform_view.hpp>
#include <boost/optional.hpp>
#include <boost/scope_exit.hpp>

#include <map>
#include <set>
#include <fstream>
#include <numeric>

#include "stylable.hpp"
#include "gradient.hpp"
#include "clip_buffer.hpp"
#include "filter.hpp"

namespace boost {
  namespace mpl {

#   define BOOST_PP_ITERATION_PARAMS_1 \
    (3,(51, 60, <boost/mpl/set/aux_/numbered.hpp>))
#   include BOOST_PP_ITERATE()

  }
}


class Transformable;
class Canvas;
class Path;
class Use;
class Switch;
class ReferencedSymbolOrSvg;
class Mask;
class Marker;

struct Document
{
  class FollowRef;

  Document(XMLDocument & xmlDocument)
    : xml_document_(xmlDocument)
    , gradients_(xml_document_)
    , filters_(xml_document_)
  {}

  XMLDocument & xml_document_;
  Gradients gradients_;
  Filters filters_;
  typedef std::set<XMLElement> followed_refs_t;
  followed_refs_t followed_refs_;
};

class Document::FollowRef
{
public:
  FollowRef(Document & document, XMLElement const & el)
    : document_(document)
  {
    std::pair<Document::followed_refs_t::iterator, bool> ins = document.followed_refs_.insert(el);
    if (!ins.second)
      throw std::runtime_error("Cyclic reference found");
    lock_ = ins.first;
  }

  ~FollowRef()
  {
    document_.followed_refs_.erase(lock_);
  }

private:
  Document & document_;
  Document::followed_refs_t::iterator lock_;
};

struct path_policy: svgpp::policy::path::raw
{
  static const bool no_ortho_line_to = true;
  static const bool no_quadratic_bezier_shorthand = true;
  static const bool no_cubic_bezier_shorthand = true;
  static const bool arc_as_cubic_bezier = true; 
};

struct child_context_factories
{
  template<class ParentContext, class ElementTag, class Enable = void>
  struct apply;
};

template<>
struct child_context_factories::apply<Canvas, svgpp::tag::element::svg, void>
{
  typedef svgpp::factory::context::on_stack<Canvas> type;
};

template<>
struct child_context_factories::apply<Canvas, svgpp::tag::element::g, void>
{
  typedef svgpp::factory::context::on_stack<Canvas> type;
};

template<>
struct child_context_factories::apply<Canvas, svgpp::tag::element::a, void>
{
  typedef svgpp::factory::context::on_stack<Canvas> type;
};

template<>
struct child_context_factories::apply<Canvas, svgpp::tag::element::switch_, void>
{
  typedef svgpp::factory::context::on_stack<Switch> type;
};

template<class ElementTag>
struct child_context_factories::apply<Switch, ElementTag, void>: apply<Canvas, ElementTag>
{};

template<>
struct child_context_factories::apply<Canvas, svgpp::tag::element::use_, void>
{
  typedef svgpp::factory::context::on_stack<Use> type;
};

template<class ElementTag>
struct child_context_factories::apply<Canvas, ElementTag, typename boost::enable_if<boost::mpl::has_key<svgpp::traits::shape_elements, ElementTag> >::type>
{
  typedef svgpp::factory::context::on_stack<Path> type;
};

// Elements referenced by 'use' element
template<>
struct child_context_factories::apply<Use, svgpp::tag::element::svg, void>
{
  typedef svgpp::factory::context::on_stack<ReferencedSymbolOrSvg> type;
};

template<>
struct child_context_factories::apply<Use, svgpp::tag::element::symbol, void>
{
  typedef svgpp::factory::context::on_stack<ReferencedSymbolOrSvg> type;
};

template<class ElementTag>
struct child_context_factories::apply<Use, ElementTag, void>: child_context_factories::apply<Canvas, ElementTag>
{};

template<class ElementTag>
struct child_context_factories::apply<ReferencedSymbolOrSvg, ElementTag, void>: child_context_factories::apply<Canvas, ElementTag>
{};

// 'mask'
template<class ElementTag>
struct child_context_factories::apply<Mask, ElementTag, void>: apply<Canvas, ElementTag>
{};

// 'marker'
template<class ElementTag>
struct child_context_factories::apply<Marker, ElementTag, void>: child_context_factories::apply<Canvas, ElementTag>
{};

struct processed_elements
  : boost::mpl::set<
      svgpp::tag::element::svg,
      svgpp::tag::element::g,
      svgpp::tag::element::switch_,
      svgpp::tag::element::a,
      svgpp::tag::element::use_,
      svgpp::tag::element::path,
      svgpp::tag::element::rect,
      svgpp::tag::element::line,
      svgpp::tag::element::circle,
      svgpp::tag::element::ellipse,
      svgpp::tag::element::polyline,
      svgpp::tag::element::polygon
    >
{};

struct processed_attributes
  : boost::mpl::set58<
    // svgpp::traits::shapes_attributes_by_element
    boost::mpl::pair<svgpp::tag::element::path, svgpp::tag::attribute::d>,
    boost::mpl::pair<svgpp::tag::element::rect, svgpp::tag::attribute::x>,
    boost::mpl::pair<svgpp::tag::element::rect, svgpp::tag::attribute::y>,
    boost::mpl::pair<svgpp::tag::element::rect, svgpp::tag::attribute::width>,
    boost::mpl::pair<svgpp::tag::element::rect, svgpp::tag::attribute::height>,
    boost::mpl::pair<svgpp::tag::element::rect, svgpp::tag::attribute::rx>,
    boost::mpl::pair<svgpp::tag::element::rect, svgpp::tag::attribute::ry>,
    boost::mpl::pair<svgpp::tag::element::circle, svgpp::tag::attribute::cx>,
    boost::mpl::pair<svgpp::tag::element::circle, svgpp::tag::attribute::cy>,
    boost::mpl::pair<svgpp::tag::element::circle, svgpp::tag::attribute::r>,
    boost::mpl::pair<svgpp::tag::element::ellipse, svgpp::tag::attribute::cx>,
    boost::mpl::pair<svgpp::tag::element::ellipse, svgpp::tag::attribute::cy>,
    boost::mpl::pair<svgpp::tag::element::ellipse, svgpp::tag::attribute::rx>,
    boost::mpl::pair<svgpp::tag::element::ellipse, svgpp::tag::attribute::ry>,
    boost::mpl::pair<svgpp::tag::element::line, svgpp::tag::attribute::x1>,
    boost::mpl::pair<svgpp::tag::element::line, svgpp::tag::attribute::y1>,
    boost::mpl::pair<svgpp::tag::element::line, svgpp::tag::attribute::x2>,
    boost::mpl::pair<svgpp::tag::element::line, svgpp::tag::attribute::y2>,
    boost::mpl::pair<svgpp::tag::element::polyline, svgpp::tag::attribute::points>,
    boost::mpl::pair<svgpp::tag::element::polygon, svgpp::tag::attribute::points>,
    // svgpp::traits::viewport_attributes
    svgpp::tag::attribute::x,
    svgpp::tag::attribute::y,
    svgpp::tag::attribute::width,
    svgpp::tag::attribute::height,
    svgpp::tag::attribute::viewBox,
    svgpp::tag::attribute::preserveAspectRatio,
    //
    svgpp::tag::attribute::display,
    svgpp::tag::attribute::transform,
    svgpp::tag::attribute::clip_path,
    svgpp::tag::attribute::color,
    svgpp::tag::attribute::fill,
    svgpp::tag::attribute::fill_opacity,
    svgpp::tag::attribute::fill_rule,
    svgpp::tag::attribute::filter,
    svgpp::tag::attribute::marker_start,
    svgpp::tag::attribute::marker_mid,
    svgpp::tag::attribute::marker_end,
    svgpp::tag::attribute::marker,
    svgpp::tag::attribute::markerUnits,
    svgpp::tag::attribute::markerWidth,
    svgpp::tag::attribute::markerHeight,
    svgpp::tag::attribute::mask,
    svgpp::tag::attribute::maskUnits,
    svgpp::tag::attribute::maskContentUnits,
    svgpp::tag::attribute::refX,
    svgpp::tag::attribute::refY,
    svgpp::tag::attribute::stroke,
    svgpp::tag::attribute::stroke_width,
    svgpp::tag::attribute::stroke_opacity,
    svgpp::tag::attribute::stroke_linecap,
    svgpp::tag::attribute::stroke_linejoin,
    svgpp::tag::attribute::stroke_miterlimit,
    svgpp::tag::attribute::stroke_dasharray,
    svgpp::tag::attribute::stroke_dashoffset,
    svgpp::tag::attribute::opacity,
    svgpp::tag::attribute::orient,
    svgpp::tag::attribute::overflow,
    boost::mpl::pair<svgpp::tag::element::use_, svgpp::tag::attribute::xlink::href>
  >
{};

class ImageBuffer: boost::noncopyable
{
public:
  ImageBuffer()
  {}

  ImageBuffer(int width, int height)
  {
    setSize(width, height, TransparentBlackColor());
  }

  int width() const { return bitmap_.width(); }
  int height() const { return bitmap_.height(); }
  SkBitmap & bitmap() { return bitmap_; }

  bool isSizeSet() const { return !bitmap_.empty(); }

  void setSize(int width, int height, color_t const & fill_color)
  {

    bitmap_.allocN32Pixels(width, height);
    bitmap_.eraseColor(fill_color); 
  }

  boost::gil::rgba8_view_t gilView()
  {
    return boost::gil::interleaved_view(bitmap_.width(), bitmap_.height(), 
      reinterpret_cast<boost::gil::rgba8_pixel_t*>(bitmap_.getPixels()), bitmap_.rowBytesAsPixels());
  }

private:
  SkBitmap bitmap_;
};

class Transformable
{
public:
  Transformable()
  {
    transform_.reset();
  }


  void transform_matrix(const boost::array<SkScalar, 6> & matrix)
  {
    SkMatrix m;
    m.setAffine(matrix.data());
    transform_.preConcat(m);
  }

  void transform_translate(SkScalar tx, SkScalar ty)
  {
    transform_.preTranslate(tx, ty);
  }

  void transform_translate(SkScalar tx)
  {
    transform_.preTranslate(tx, 0);
  }

  void transform_scale(SkScalar sx, SkScalar sy)
  {
    transform_.preScale(sx, sy);
  }

  void transform_scale(SkScalar scale)
  {
    transform_.preScale(scale, scale);
  }

  void transform_rotate(SkScalar angle)
  {
    transform_.preRotate(angle);
  }

  void transform_rotate(SkScalar angle, SkScalar cx, SkScalar cy)
  {
    transform_.preRotate(angle, cx, cy);
  }

  void transform_skew_x(SkScalar angle)
  {
    transform_.preSkew(std::tan(angle * boost::math::constants::degree<SkScalar>()), 0);
  }

  void transform_skew_y(SkScalar angle)
  {
    transform_.preSkew(0, std::tan(angle * boost::math::constants::degree<SkScalar>()));
  }

  transform_t       & transform()       { return transform_; }
  transform_t const & transform() const { return transform_; }

private:
  transform_t transform_;
};

typedef boost::function<ImageBuffer&()> lazy_buffer_t;

namespace
{
  template<class GrayMask>
  void blend_image_with_mask(boost::gil::rgba8_view_t const & rgbaView, GrayMask const & maskView)
  {
    boost::gil::rgba8_view_t::iterator o = rgbaView.begin();
    for(typename GrayMask::iterator m = maskView.begin(); m != maskView.end(); ++m, ++o)
    {
      using namespace boost::gil;
      get_color(*o, alpha_t()) = 
        channel_multiply(
          get_color(*o, alpha_t()),
          get_color(*m, gray_color_t())
        );
    }
    BOOST_ASSERT(o == rgbaView.end());
  }
}

class Canvas: 
  public Stylable,
  public Transformable
{
public:
  struct dontInheritStyle {};

  Canvas(Document & document, ImageBuffer & image_buffer)
    : document_(document)
    , parent_buffer_(boost::bind(&Canvas::getPassedImageBuffer, this))
    , image_buffer_(&image_buffer)
    , rendering_disabled_(false)
  {
    if (image_buffer.isSizeSet())
      clip_buffer_.reset(new ClipBuffer(image_buffer_->width(), image_buffer_->height()));
  }

  Canvas(Canvas & parent)
    : Transformable(parent)
    , Stylable(parent)
    , document_(parent.document_)
    , image_buffer_(NULL)
    , parent_buffer_(boost::bind(&Canvas::getImageBuffer, &parent))
    , length_factory_(parent.length_factory_)
    , clip_buffer_(parent.clip_buffer_)
    , rendering_disabled_(false)
  {}

  Canvas(Canvas & parent, dontInheritStyle)
    : Transformable(parent)
    , document_(parent.document_)
    , image_buffer_(NULL)
    , parent_buffer_(boost::bind(&Canvas::getImageBuffer, &parent))
    , length_factory_(parent.length_factory_)
    , clip_buffer_(parent.clip_buffer_)
    , rendering_disabled_(false)
  {}

  void on_exit_element()
  {
    if (!own_buffer_.get())
      return;
    applyFilter();

    if (style().clip_path_fragment_)
    {
      if (!clip_buffer_.unique())
        clip_buffer_.reset(new ClipBuffer(*clip_buffer_));
      clip_buffer_->intersectClipPath(document().xml_document_, *style().clip_path_fragment_, transform());
    }

    if (clip_buffer_)
      blend_image_with_mask(own_buffer_->gilView(), clip_buffer_->gilView());

    if (style().mask_fragment_)
    {
      ImageBuffer & parent_buffer = parent_buffer_();
      ImageBuffer mask_buffer(parent_buffer.width(), parent_buffer.height());
      loadMask(mask_buffer);
      typedef boost::gil::color_converted_view_type<
        boost::gil::rgba8_view_t, 
        boost::gil::gray8_pixel_t, 
        svgpp::gil_utility::rgba_to_mask_color_converter<> 
      >::type mask_view_t;
      mask_view_t mask_view = boost::gil::color_converted_view<boost::gil::gray8_pixel_t>(
        mask_buffer.gilView(), svgpp::gil_utility::rgba_to_mask_color_converter<>());

      blend_image_with_mask(own_buffer_->gilView(), mask_view);
    }

    {
      SkCanvas canvas(parent_buffer_().bitmap());
      SkPaint paint;
      paint.setStyle(SkPaint::kFill_Style);
      paint.setAlpha(style().opacity_ * 255);
      canvas.drawBitmap(own_buffer_->bitmap(), 0, 0, &paint);
    }
  }

  void set_viewport(number_t viewport_x, number_t viewport_y, number_t viewport_width, number_t viewport_height)
  {
    if (image_buffer_) // If topmost SVG element
    {
      image_buffer_->setSize(viewport_width + 1.0, viewport_height + 1.0, TransparentWhiteColor());
      clip_buffer_.reset(new ClipBuffer(image_buffer_->width(), image_buffer_->height()));
    }
    else
    {
      if (style().overflow_clip_)
      {
        if (!clip_buffer_.unique())
          clip_buffer_.reset(new ClipBuffer(*clip_buffer_));
        clip_buffer_->intersectClipRect(transform(), viewport_x, viewport_y, viewport_width, viewport_height);
      }
    }
    length_factory_.set_viewport_size(viewport_width, viewport_height);
  }

  void set_viewbox_size(number_t viewbox_width, number_t viewbox_height)
  {
    // If "viewbox" attribute is present, then percentage is calculated relative to viewbox
    length_factory_.set_viewport_size(viewbox_width, viewbox_height);
  }

  void disable_rendering()
  { rendering_disabled_ = true; }

  bool rendering_disabled() const
  { return rendering_disabled_; }

  length_factory_t & length_factory() 
  { return length_factory_; }

  length_factory_t const & length_factory() const
  { return length_factory_; }

private:
  Document & document_;
  ImageBuffer * const image_buffer_; // Non-NULL only for topmost SVG element
  lazy_buffer_t parent_buffer_;
  std::auto_ptr<ImageBuffer> own_buffer_;
  boost::shared_ptr<ClipBuffer> clip_buffer_;
  length_factory_t length_factory_;
  bool rendering_disabled_;

  void loadMask(ImageBuffer &) const;
  void applyFilter();
  ImageBuffer & getPassedImageBuffer() { return *image_buffer_; }

protected:
  ImageBuffer & getImageBuffer()
  {
    ImageBuffer & parent_buffer = parent_buffer_();

    if (style().opacity_ < 0.999 
      || style().mask_fragment_ 
      || style().clip_path_fragment_ 
      || style().filter_)
    {
      if (!own_buffer_.get())
        own_buffer_.reset(new ImageBuffer(parent_buffer.width(), parent_buffer.height()));
      return *own_buffer_;
    }
    return parent_buffer;
  }

  Document & document() const { return document_; }
  ClipBuffer const & clipBuffer() const { return *clip_buffer_; }
  virtual bool isSwitchElement() const { return false; }
};

struct SimpleFilterView: IFilterView
{
  SimpleFilterView(boost::gil::rgba8c_view_t const & v)
    : view_(v)
  {}

  virtual boost::gil::rgba8c_view_t view() { return view_; }

private:
  boost::gil::rgba8c_view_t view_;
};

void Canvas::applyFilter()
{
  if (!style().filter_)
    return;

  Filters::Input in;
  in.sourceGraphic_ = IFilterViewPtr(new SimpleFilterView(own_buffer_->gilView()));
  in.backgroundImage_ = IFilterViewPtr(new SimpleFilterView(parent_buffer_().gilView()));
  IFilterViewPtr out = document_.filters_.get(*style().filter_, length_factory_, in);
  if (out)
    boost::gil::copy_pixels(out->view(), own_buffer_->gilView());
}

class Switch: public Canvas
{
public:
  Switch(Canvas & parent)
    : Canvas(parent)
  {}

  virtual bool isSwitchElement() const { return true; }
};

class Path: 
  public Canvas
{
public:
  Path(Canvas & parent)
    : Canvas(parent)
  {}

  void on_exit_element()
  {
    if (style().display_)
    {
      drawPath();
      drawMarkers();
    }
    Canvas::on_exit_element();
  }


  void path_move_to(SkScalar x, SkScalar y, svgpp::tag::coordinate::absolute const &)
  { 
    path_.moveTo(x, y);
  }

  void path_move_to(SkScalar x, SkScalar y, svgpp::tag::coordinate::relative const &)
  { 
    path_.rMoveTo(x, y);
  }

  void path_line_to(SkScalar x, SkScalar y, svgpp::tag::coordinate::absolute const &)
  { 
    path_.lineTo(x, y);
  }

  void path_line_to(SkScalar x, SkScalar y, svgpp::tag::coordinate::relative const &)
  { 
    path_.rLineTo(x, y);
  }

  void path_cubic_bezier_to(
    SkScalar x1, SkScalar y1, 
    SkScalar x2, SkScalar y2, 
    SkScalar x, SkScalar y, 
    svgpp::tag::coordinate::absolute const &)
  { 
    path_.cubicTo(x1, y1, x2, y2, x, y);
  }

  void path_cubic_bezier_to(
    SkScalar x1, SkScalar y1, 
    SkScalar x2, SkScalar y2, 
    SkScalar x, SkScalar y, 
    svgpp::tag::coordinate::relative const &)
  { 
    path_.rCubicTo(x1, y1, x2, y2, x, y);
  }

  void path_quadratic_bezier_to(
    SkScalar x1, SkScalar y1, 
    SkScalar x, SkScalar y, 
    svgpp::tag::coordinate::absolute const &)
  { 
    path_.quadTo(x1, y1, x, y);
  }

  void path_quadratic_bezier_to(
    SkScalar x1, SkScalar y1, 
    SkScalar x, SkScalar y, 
    svgpp::tag::coordinate::relative const &)
  { 
    path_.rQuadTo(x1, y1, x, y);
  }

  void path_close_subpath()
  {
    path_.close();
  }

  void path_exit()
  {}

  void marker(svgpp::marker_vertex v, number_t x, number_t y, number_t directionality, unsigned marker_index)
  {
    if (marker_index >= markers_.size())
      markers_.resize(marker_index + 1);
    MarkerPos & m = markers_[marker_index];
    m.v = v;
    m.x = x;
    m.y = y;
    m.directionality = directionality;
  }

private:
  SkPath path_;

  struct MarkerPos
  {
    svgpp::marker_vertex v;
    number_t x, y, directionality;
  };

  typedef std::vector<MarkerPos> Markers; 
  Markers markers_;

  boost::optional<svg_string_t> & getMarkerReference(svgpp::marker_vertex v)
  {
    switch (v)
    {
    default:
      BOOST_ASSERT(false);
    case svgpp::marker_start:
      return style().marker_start_;
    case svgpp::marker_mid:
      return style().marker_mid_;
    case svgpp::marker_end:
      return style().marker_end_;
    }
  }

  typedef boost::variant<svgpp::tag::value::none, color_t, Gradient> EffectivePaint;
  void drawPath();
  void drawMarkers();
  void drawMarker(svg_string_t const & id, number_t x, number_t y, number_t dir);
  EffectivePaint getEffectivePaint(Paint const &) const;
};

struct afterMarkerUnitsTag {};

struct attribute_traversal: svgpp::policy::attribute_traversal::default_policy
{
  typedef boost::mpl::if_<
    boost::is_same<boost::mpl::_1, svgpp::tag::element::marker>,
    boost::mpl::vector<
      svgpp::tag::attribute::markerUnits,
      svgpp::tag::attribute::orient,
      svgpp::notify_context<afterMarkerUnitsTag>
    >::type,
    boost::mpl::empty_sequence
  > get_priority_attributes_by_element;
};

struct DocumentTraversalControl
{
  static bool proceed_to_element_content(Canvas const & canvas)
  {
    return canvas.style().display_ && !canvas.rendering_disabled();
  }

  template<class Context>
  static bool proceed_to_next_child(Context &)
  {
    return true;
  }
};

typedef 
  svgpp::document_traversal<
    svgpp::number_type<SkScalar>,
    svgpp::transform_policy<svgpp::policy::transform::raw>,
    svgpp::context_factories<child_context_factories>,
    svgpp::length_policy<svgpp::policy::length::forward_to_method<Canvas, const length_factory_t> >,
    svgpp::color_factory<color_factory_t>,
    svgpp::processed_elements<processed_elements>,
    svgpp::processed_attributes<processed_attributes>,
    svgpp::path_policy<path_policy>,
    svgpp::document_traversal_control_policy<DocumentTraversalControl>,
    svgpp::transform_events_policy<svgpp::policy::transform_events::forward_to_method<Transformable> >, // Same as default, but less instantiations
    svgpp::path_events_policy<svgpp::policy::path_events::forward_to_method<Path> >, // Same as default, but less instantiations
    svgpp::error_policy<svgpp::policy::error::default_policy<Stylable> >, // Type of context isn't used
    svgpp::markers_policy<svgpp::policy::markers::calculate_always>,
    svgpp::attribute_traversal_policy<attribute_traversal>,
    svgpp::viewport_policy<svgpp::policy::viewport::as_transform>
  > document_traversal_main;

class Use: public Canvas
{
public:
  Use(Canvas & parent)
    : Canvas(parent)
    , x_(0)
    , y_(0)
  {}

  void on_exit_element()
  {
    if (!style().display_)
      return;
    if (XMLElement element = document().xml_document_.findElementById(fragment_id_))
    {
      Document::FollowRef lock(document(), element);
      transform().preTranslate(x_, y_);
      document_traversal_main::load_referenced_element<
        svgpp::referencing_element<svgpp::tag::element::use_>,
        svgpp::expected_elements<svgpp::traits::reusable_elements>,
        svgpp::processed_elements<
          boost::mpl::insert<processed_elements, svgpp::tag::element::symbol>::type 
        >
      >::load(element, *this);
    }
    else
      std::cerr << "Element referenced by 'use' not found\n";
    Canvas::on_exit_element();
  }

  using Canvas::set;

  template<class IRI>
  void set(svgpp::tag::attribute::xlink::href, svgpp::tag::iri_fragment, IRI const & fragment)
  { fragment_id_.assign(boost::begin(fragment), boost::end(fragment)); }

  template<class IRI>
  void set(svgpp::tag::attribute::xlink::href, IRI const & fragment)
  { std::cerr << "External references aren't supported\n"; }

  void set(svgpp::tag::attribute::x, number_t val)
  { x_ = val; }

  void set(svgpp::tag::attribute::y, number_t val)
  { y_ = val; }

  void set(svgpp::tag::attribute::width, number_t val)
  { width_ = val; }

  void set(svgpp::tag::attribute::height, number_t val)
  { height_ = val; }

  boost::optional<number_t> const & width() const { return width_; }
  boost::optional<number_t> const & height() const { return height_; }

private:
  svg_string_t fragment_id_;
  number_t x_, y_;
  boost::optional<number_t> width_, height_;
};

class ReferencedSymbolOrSvg: 
  public Canvas
{
public:
  ReferencedSymbolOrSvg(Use & parent)
    : Canvas(parent)
    , parent_(parent)
  {
  }

  void get_reference_viewport_size(number_t & width, number_t & height)
  {
    if (parent_.width())
      width = *parent_.width();
    if (parent_.height())
      height = *parent_.height();
  }

private:
  Use & parent_;
};

class Mask: public Canvas
{
public:
  Mask(Document & document, ImageBuffer & image_buffer, Transformable const & referenced)
    : Canvas(document, image_buffer)
    , maskUseObjectBoundingBox_(true)
    , maskContentUseObjectBoundingBox_(false)
  {
#if defined(RENDERER_AGG) || defined(RENDERER_SKIA)
    transform() = referenced.transform();
#elif defined(RENDERER_GDIPLUS)
    AssignMatrix(transform(), referenced.transform());
#endif
  }

  void on_enter_element(svgpp::tag::element::mask) 
  {}

  void on_exit_element()
  {}

  using Canvas::set;

  void set(svgpp::tag::attribute::maskUnits, svgpp::tag::value::userSpaceOnUse)
  { maskUseObjectBoundingBox_ = false; }

  void set(svgpp::tag::attribute::maskUnits, svgpp::tag::value::objectBoundingBox)
  { maskUseObjectBoundingBox_ = true; }

  void set(svgpp::tag::attribute::maskContentUnits, svgpp::tag::value::userSpaceOnUse)
  { maskContentUseObjectBoundingBox_ = false; }

  void set(svgpp::tag::attribute::maskContentUnits, svgpp::tag::value::objectBoundingBox)
  { maskContentUseObjectBoundingBox_ = true; }

  void set(svgpp::tag::attribute::x, number_t val)
  { x_ = val; }

  void set(svgpp::tag::attribute::y, number_t val)
  { y_ = val; }

  void set(svgpp::tag::attribute::width, number_t val)
  { width_ = val; }

  void set(svgpp::tag::attribute::height, number_t val)
  { height_ = val; }

private:
  bool maskUseObjectBoundingBox_, maskContentUseObjectBoundingBox_;
  number_t x_, y_, width_, height_; // TODO: defaults
};

void Canvas::loadMask(ImageBuffer & mask_buffer) const
{
  if (XMLElement element = document().xml_document_.findElementById(*style().mask_fragment_))
  {
    Document::FollowRef lock(document(), element);

    Mask mask(document_, mask_buffer, *this);
    document_traversal_main::load_expected_element(element, mask, svgpp::tag::element::mask());
  }
  else
    throw std::runtime_error("Element referenced by 'mask' not found");
}

struct GradientBase_visitor: boost::static_visitor<>
{
  void operator()(GradientBase const & g) 
  {
    gradient_ = &g;
  }

  GradientBase const * gradient_;
};

void AssignGradientPaint(SkPaint & paint, SkPath const & path, Gradient const & gradient, SkMatrix transform)
{
  GradientBase_visitor gradientBase;
  boost::apply_visitor(gradientBase, gradient);

  std::vector<SkColor> colors;
  std::vector<SkScalar> positions;
  colors.reserve(gradientBase.gradient_->stops_.size());
  positions.reserve(gradientBase.gradient_->stops_.size());
  for(GradientStops::const_iterator s = gradientBase.gradient_->stops_.begin(); s != gradientBase.gradient_->stops_.end(); ++s)
  {
    colors.push_back(s->color_);
    positions.push_back(s->offset_);
  }
  BOOST_ASSERT(!colors.empty()); // Checked in GetEffectivePaint

  if (gradientBase.gradient_->useObjectBoundingBox_)
  {
    SkRect const & bounds = path.getBounds();
    if (bounds.isEmpty())
      return;
    transform.preTranslate(bounds.x(), bounds.y());
    transform.preScale(bounds.width(), bounds.height());
  }

  if (gradientBase.gradient_->matrix_)
    transform.preConcat(*gradientBase.gradient_->matrix_);

  if (LinearGradient const * linearGradient = boost::get<LinearGradient>(&gradient))
  {
    const SkPoint pts[2] = 
    {
      SkPoint::Make(linearGradient->x1_, linearGradient->y1_),
      SkPoint::Make(linearGradient->x2_, linearGradient->y2_)
    };
    paint.setShader(
      SkGradientShader::CreateLinear(
        pts, &colors[0], &positions[0], positions.size(), linearGradient->spreadMethod_,
        0, &transform
      )
    )->unref();
  }
  else
  {
    RadialGradient const & radialGradient = boost::get<RadialGradient>(gradient);
    if (radialGradient.fx_ == radialGradient.cx_ && radialGradient.fy_ == radialGradient.cy_)
      paint.setShader(
        SkGradientShader::CreateRadial(
          SkPoint::Make(radialGradient.cx_, radialGradient.cy_), 
          radialGradient.r_,
          &colors[0], &positions[0], positions.size(), radialGradient.spreadMethod_,
          0, &transform
        )
      )->unref();
    else
      paint.setShader(
        SkGradientShader::CreateTwoPointRadial(
          SkPoint::Make(radialGradient.fx_, radialGradient.fy_), 
          0,
          SkPoint::Make(radialGradient.cx_, radialGradient.cy_), 
          radialGradient.r_,
          &colors[0], &positions[0], positions.size(), radialGradient.spreadMethod_,
          0, &transform
        )
      )->unref();
  }
}

void Path::drawPath()
{
  if (path_.isEmpty())
    return;
  path_.setFillType(style().nonzero_fill_rule_ ? SkPath::kWinding_FillType : SkPath::kEvenOdd_FillType);

  SkCanvas canvas(getImageBuffer().bitmap());
  canvas.setMatrix(transform());
  EffectivePaint fill = getEffectivePaint(style().fill_paint_);
  if (boost::get<svgpp::tag::value::none>(&fill) == NULL)
  {
    SkPaint fillPaint;
    fillPaint.setAntiAlias(true);
    fillPaint.setStyle(SkPaint::kFill_Style);
    if (color_t const * color = boost::get<color_t>(&fill))
    {
      fillPaint.setColor(SkColorSetA(*color, style().fill_opacity_ * 255));
    }
    else 
    {
      fillPaint.setColor(SkColorSetA(0, style().fill_opacity_ * 255));
      AssignGradientPaint(fillPaint, path_, boost::get<Gradient const>(fill), transform());
    }

    canvas.drawPath(path_, fillPaint);
  }
  EffectivePaint stroke = getEffectivePaint(style().stroke_paint_);
  if (boost::get<svgpp::tag::value::none>(&stroke) == NULL)
  {
    SkPaint strokePaint = style().skPaintStroke_;
    strokePaint.setAlpha(style().stroke_opacity_ * 255);
    std::vector<number_t> const & dasharray = style().stroke_dasharray_;
    if (!dasharray.empty())
    {
      std::vector<SkScalar> dashes(dasharray.begin(), dasharray.end());
      if (dasharray.size() % 2 == 1)
        dashes.insert(dashes.end(), dasharray.begin(), dasharray.end());
      strokePaint.setPathEffect(SkDashPathEffect::Create(&dashes[0], dashes.size(), style().stroke_dashoffset_))->unref();
    }
    if (color_t const * color = boost::get<color_t>(&stroke))
    {
      strokePaint.setColor(SkColorSetA(*color, style().stroke_opacity_ * 255));
    }
    else
    {
      strokePaint.setColor(SkColorSetA(0, style().stroke_opacity_ * 255));
      AssignGradientPaint(strokePaint, path_, boost::get<Gradient const>(stroke), transform());
    }
    canvas.drawPath(path_, strokePaint);
  }
}

class Marker: 
  public Canvas
{
public:
  Marker(Path & parent, number_t strokeWidth, number_t x, number_t y, number_t autoOrient)
    : Canvas(parent, dontInheritStyle())
    , autoOrient_(autoOrient)
    , strokeWidth_(strokeWidth)
    , orient_(0.0)
    , strokeWidthUnits_(true)
  {
    transform().preTranslate(x, y);
  }

  void on_enter_element(svgpp::tag::element::marker) {}
  void on_exit_element() {}

  bool notify(afterMarkerUnitsTag)
  {
    // strokeWidthUnits_ and orient_ already set
    if (strokeWidthUnits_)
    {
      length_factory() = length_factory_t();

      transform().preScale(strokeWidth_, strokeWidth_);
    }
    transform().preRotate(orient_ * boost::math::constants::radian<number_t>());

    return true;
  }

  using Canvas::set;

  void set(svgpp::tag::attribute::markerUnits, svgpp::tag::value::strokeWidth)
  { strokeWidthUnits_ = true; }

  void set(svgpp::tag::attribute::markerUnits, svgpp::tag::value::userSpaceOnUse)
  { strokeWidthUnits_ = false; }

  void set(svgpp::tag::attribute::orient, number_t val)
  { orient_ = val * boost::math::constants::degree<number_t>(); }

  void set(svgpp::tag::attribute::orient, svgpp::tag::value::auto_)
  { orient_ = autoOrient_; }

private:
  number_t const strokeWidth_;
  number_t const autoOrient_;
  bool strokeWidthUnits_;
  number_t orient_;
};

void Path::drawMarkers()
{
  if (!style().marker_start_ && !style().marker_mid_ && !style().marker_end_)
    return;
  for(Markers::const_iterator pos = markers_.begin(); pos != markers_.end(); ++pos)
  {
    if (boost::optional<svg_string_t> & m = getMarkerReference(pos->v))
    {
      drawMarker(*m, pos->x, pos->y, pos->directionality);
    }
  }
}

void Path::drawMarker(svg_string_t const & id, number_t x, number_t y, number_t dir)
{
  if (XMLElement element = document().xml_document_.findElementById(id))
  {
    Document::FollowRef lock(document(), element);

    Marker markerContext(*this, 
      style().skPaintStroke_.getStrokeWidth(), 
      x, y, dir);
    document_traversal_main::load_expected_element(element, markerContext, svgpp::tag::element::marker());
  }
}

Path::EffectivePaint Path::getEffectivePaint(Paint const & paint) const
{
  SolidPaint const * solidPaint = NULL;
  if (IRIPaint const * iri = boost::get<IRIPaint>(&paint))
  {
    if (boost::optional<Gradient> const gradient = document().gradients_.get(iri->fragment_, length_factory()))
    {
      GradientBase_visitor gradientBase;
      boost::apply_visitor(gradientBase, *gradient);
      if (gradientBase.gradient_->stops_.empty())
        return svgpp::tag::value::none();
      if (gradientBase.gradient_->stops_.size() == 1)
        return gradientBase.gradient_->stops_.front().color_;
      if (LinearGradient const * linearGradient = boost::get<LinearGradient>(gradient.get_ptr()))
      {
        if (linearGradient->x1_ == linearGradient->x2_ && linearGradient->y1_ == linearGradient->y2_)
          // TODO: use also last step opacity 
          return gradientBase.gradient_->stops_.back().color_;
      }
      return *gradient;
    }
    if (iri->fallback_)
      solidPaint = &*iri->fallback_;
    else
      throw std::runtime_error("Can't find paint server");
  }
  else
    solidPaint = boost::get<SolidPaint>(&paint);
  if (boost::get<svgpp::tag::value::none>(solidPaint))
    return svgpp::tag::value::none();
  if (boost::get<svgpp::tag::value::currentColor>(solidPaint))
    return style().color_;
  return boost::get<color_t>(*solidPaint);
}

void renderDocument(XMLDocument & xmlDocument, ImageBuffer & buffer)
{
  Document document(xmlDocument);
  Canvas canvas(document, buffer);
  document_traversal_main::load_document(xmlDocument.getRoot(), canvas);
}

int main(int argc, char * argv[])
{
  if (argc < 2)
  {
    std::cout << "Usage: " << argv[0] << " <svg file name> [<output BMP file name>]\n";
    return 1;
  }

  {
    ImageBuffer buffer;
  
    XMLDocument xmlDoc;
    try
    {
      xmlDoc.load(argv[1]);
      renderDocument(xmlDoc, buffer);
    }
    catch(svgpp::exception_base const & e)
    {
      typedef boost::error_info<svgpp::tag::error_info::xml_element, XMLElement> element_error_info;
      std::cerr << "Error reading file " << argv[1];
#if defined(SVG_PARSER_RAPIDXML_NS)
      if (XMLElement const * element = boost::get_error_info<element_error_info>(e))
        std::cerr << " in element \"" << std::string((*element)->name(), (*element)->name() + (*element)->name_size())
          << "\"";
#endif
      std::cerr << ": " << e.what() << "\n";
      if (!buffer.isSizeSet()) {
        // Buffer isn't created yet - nothing to save, exiting.
        return 1;
      }
    }
    catch(std::exception const & e)
    {
      std::cerr << "Error reading file " << argv[1] << ": " << e.what() << "\n";
      return 1;
    }

    // Saving output
    const char * out_file_name = argc > 2 ? argv[2] : "svgpp.png";

    __SK_FORCE_IMAGE_DECODER_LINKING;
    if (!SkImageEncoder::EncodeFile(out_file_name, buffer.bitmap(), SkImageEncoder::kPNG_Type, 100))
      std::cerr << "Failed to write " << out_file_name << "\n";
}
  return 0;
}