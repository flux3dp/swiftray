#include <parser/svgpp_common.h>

class StylableContext {
public:
  StylableContext() : stroke_width_(1) {}

  void set(tag::attribute::stroke_width, double val) { stroke_width_ = val; }

  void set(tag::attribute::stroke, tag::value::none) {
    stroke_ = tag::value::none();
  }

  void set(tag::attribute::stroke, tag::value::currentColor) {
    stroke_ = tag::value::currentColor();
  }

  void set(tag::attribute::stroke, color_t color,
           tag::skip_icc_color = tag::skip_icc_color()) {
    stroke_ = color;
    qInfo() << "Set stroke" << color.get<0>() << color.get<1>()
            << color.get<2>();
  }

  template <class IRI> void set(tag::attribute::stroke tag, IRI const &iri) {
    throw std::runtime_error("Non-local references aren't supported");
  }

  template <class IRI>
  void set(tag::attribute::stroke tag, tag::iri_fragment, IRI const &fragment) {
    stroke_ =
        IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)));
  }

  template <class IRI>
  void set(tag::attribute::stroke tag, IRI const &, tag::value::none val) {
    set(tag, val);
  }

  template <class IRI>
  void set(tag::attribute::stroke tag, tag::iri_fragment, IRI const &fragment,
           tag::value::none val) {
    stroke_ =
        IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)),
                 boost::optional<SolidPaint>(val));
  }

  template <class IRI>
  void set(tag::attribute::stroke tag, IRI const &,
           tag::value::currentColor val) {
    set(tag, val);
  }

  template <class IRI>
  void set(tag::attribute::stroke tag, tag::iri_fragment, IRI const &fragment,
           tag::value::currentColor val) {
    stroke_ =
        IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)),
                 boost::optional<SolidPaint>(val));
  }

  template <class IRI>
  void set(tag::attribute::stroke tag, IRI const &, color_t val,
           tag::skip_icc_color = tag::skip_icc_color()) {
    set(tag, val);
  }

  template <class IRI>
  void set(tag::attribute::stroke tag, tag::iri_fragment, IRI const &fragment,
           color_t val, tag::skip_icc_color = tag::skip_icc_color()) {
    stroke_ =
        IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)),
                 boost::optional<SolidPaint>(val));
  }

  void set(tag::attribute::fill, tag::value::none) {
    stroke_ = tag::value::none();
  }

  void set(tag::attribute::fill, tag::value::currentColor) {
    stroke_ = tag::value::currentColor();
  }

  void set(tag::attribute::fill, color_t color,
           tag::skip_icc_color = tag::skip_icc_color()) {
    stroke_ = color;
  }

  template <class IRI> void set(tag::attribute::fill tag, IRI const &iri) {
    throw std::runtime_error("Non-local references aren't supported");
  }

  template <class IRI>
  void set(tag::attribute::fill tag, tag::iri_fragment, IRI const &fragment) {
    stroke_ =
        IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)));
  }

  template <class IRI>
  void set(tag::attribute::fill tag, IRI const &, tag::value::none val) {
    set(tag, val);
  }

  template <class IRI>
  void set(tag::attribute::fill tag, tag::iri_fragment, IRI const &fragment,
           tag::value::none val) {
    stroke_ =
        IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)),
                 boost::optional<SolidPaint>(val));
  }

  template <class IRI>
  void set(tag::attribute::fill tag, IRI const &,
           tag::value::currentColor val) {
    set(tag, val);
  }

  template <class IRI>
  void set(tag::attribute::fill tag, tag::iri_fragment, IRI const &fragment,
           tag::value::currentColor val) {
    stroke_ =
        IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)),
                 boost::optional<SolidPaint>(val));
  }

  template <class IRI>
  void set(tag::attribute::fill tag, IRI const &, color_t val,
           tag::skip_icc_color = tag::skip_icc_color()) {
    set(tag, val);
  }

  template <class IRI>
  void set(tag::attribute::fill tag, tag::iri_fragment, IRI const &fragment,
           color_t val, tag::skip_icc_color = tag::skip_icc_color()) {
    stroke_ =
        IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)),
                 boost::optional<SolidPaint>(val));
  }

  void set(svgpp::tag::attribute::markerUnits, svgpp::tag::value::strokeWidth) {
    strokeWidthUnits_ = true;
  }

  void set(svgpp::tag::attribute::markerUnits,
           svgpp::tag::value::userSpaceOnUse) {
    strokeWidthUnits_ = false;
  }

  void set(svgpp::tag::attribute::orient, double val) {
    orient_ = val * boost::math::constants::degree<double>();
  }

  void set(svgpp::tag::attribute::orient, svgpp::tag::value::auto_) {
    orient_ = autoOrient_;
  }

protected:
  Paint stroke_;
  Paint fill_;
  double stroke_width_;
  double const strokeWidth_ = 1;
  double const autoOrient_ = 1;
  bool strokeWidthUnits_;
  double orient_;
};
