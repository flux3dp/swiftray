#include <QDebug>
#include <parser/svgpp_common.h>
#pragma once

using namespace svgpp;

inline color_t BlackColor() { return color_t(0, 0, 0); }
inline color_t TransparentBlackColor() { return color_t(0, 0, 0); }
inline color_t TransparentWhiteColor() { return color_t(255, 255, 255); }

#define BUTTER_CAP 1
#define MITER_JOIN 1
struct InheritedStyle {
    InheritedStyle()
        : color_(BlackColor()), fill_paint_(BlackColor()),
          stroke_paint_(tag::value::none()), nonzero_fill_rule_(true),
          stroke_opacity_(1.0), fill_opacity_(1.0), stroke_width_(1.0),
          line_cap_(BUTTER_CAP), line_join_(MITER_JOIN), miterlimit_(4.0),
          stroke_dashoffset_(0) {}

    color_t color_;
    SVGPPPaint fill_paint_, stroke_paint_;
    double stroke_opacity_, fill_opacity_;
    bool nonzero_fill_rule_;
    double stroke_width_;
    int line_cap_;
    int line_join_;
    double miterlimit_;
    std::vector<double> stroke_dasharray_;
    double stroke_dashoffset_;
    boost::optional<std::string> marker_start_, marker_mid_, marker_end_;
};

struct NoninheritedStyle {
    NoninheritedStyle() : opacity_(1.0), display_(true), overflow_clip_(true) {}

    double opacity_;
    bool display_;
    boost::optional<std::string> mask_fragment_, clip_path_fragment_;
    boost::optional<std::string> filter_;
    bool overflow_clip_;
};

struct Style : InheritedStyle, NoninheritedStyle {
    struct inherit_tag {};

    Style() {}

    Style(Style const &src, inherit_tag) : InheritedStyle(src) {}
};

template <class AttributeTag>
class PaintContext {
  public:
    PaintContext(SVGPPPaint &paint) : paint_(paint) {}

    void set(AttributeTag, svgpp::tag::value::none) {
        paint_ = svgpp::tag::value::none();
    }

    void set(AttributeTag, svgpp::tag::value::currentColor) {
        paint_ = svgpp::tag::value::currentColor();
    }

    void set(AttributeTag, color_t color,
             svgpp::tag::skip_icc_color = svgpp::tag::skip_icc_color()) {
        paint_ = color;
    }

    template <class IRI>
    void set(AttributeTag tag, IRI const &iri) {
        throw std::runtime_error("Non-local references aren't supported");
    }

    template <class IRI>
    void set(AttributeTag tag, svgpp::tag::iri_fragment, IRI const &fragment) {
        paint_ =
            IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)));
    }

    template <class IRI>
    void set(AttributeTag tag, IRI const &, svgpp::tag::value::none val) {
        set(tag, val);
    }

    template <class IRI>
    void set(AttributeTag tag, svgpp::tag::iri_fragment, IRI const &fragment,
             svgpp::tag::value::none val) {
        paint_ =
            IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)),
                     boost::optional<SolidPaint>(val));
    }

    template <class IRI>
    void set(AttributeTag tag, IRI const &,
             svgpp::tag::value::currentColor val) {
        set(tag, val);
    }

    template <class IRI>
    void set(AttributeTag tag, svgpp::tag::iri_fragment, IRI const &fragment,
             svgpp::tag::value::currentColor val) {
        paint_ =
            IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)),
                     boost::optional<SolidPaint>(val));
    }

    template <class IRI>
    void set(AttributeTag tag, IRI const &, color_t val,
             svgpp::tag::skip_icc_color = svgpp::tag::skip_icc_color()) {
        set(tag, val);
    }

    template <class IRI>
    void set(AttributeTag tag, svgpp::tag::iri_fragment, IRI const &fragment,
             color_t val,
             svgpp::tag::skip_icc_color = svgpp::tag::skip_icc_color()) {
        paint_ =
            IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)),
                     boost::optional<SolidPaint>(val));
    }

  protected:
    SVGPPPaint &paint_;
};

class StylableContext : public PaintContext<svgpp::tag::attribute::stroke>,
                        public PaintContext<svgpp::tag::attribute::fill> {
  public:
    typedef PaintContext<svgpp::tag::attribute::stroke> stroke_paint;
    typedef PaintContext<svgpp::tag::attribute::fill> fill_paint;

    StylableContext()
        : stroke_paint(style_.stroke_paint_), fill_paint(style_.fill_paint_) {}

    StylableContext(StylableContext const &src)
        : stroke_paint(style_.stroke_paint_), fill_paint(style_.fill_paint_),
          style_(src.style_, Style::inherit_tag()) {}

    using fill_paint::set;
    using stroke_paint::set;

    void set(svgpp::tag::attribute::display, svgpp::tag::value::none) {
        style().display_ = false;
    }

    void set(svgpp::tag::attribute::display, svgpp::tag::value::inherit) {
        style().display_ = parentStyle_.display_;
    }

    template <class ValueTag>
    void set(svgpp::tag::attribute::display, ValueTag) {
        style().display_ = true;
    }

    void set(svgpp::tag::attribute::color, color_t val) {
        style().color_ = val;
    }

    void set(svgpp::tag::attribute::stroke_width, double val) {
        style().stroke_width_ = val;
    }

    void set(svgpp::tag::attribute::stroke_opacity, double val) {
        style().stroke_opacity_ = std::min(double(1), std::max(double(0), val));
    }

    void set(svgpp::tag::attribute::fill_opacity, double val) {
        style().fill_opacity_ = std::min(double(1), std::max(double(0), val));
    }

    void set(svgpp::tag::attribute::opacity, double val) {
        style().opacity_ = std::min(double(1), std::max(double(0), val));
    }

    void set(svgpp::tag::attribute::opacity, svgpp::tag::value::inherit) {
        style().opacity_ = parentStyle_.opacity_;
    }

    void set(svgpp::tag::attribute::fill_rule, svgpp::tag::value::nonzero) {
        style().nonzero_fill_rule_ = true;
    }

    void set(svgpp::tag::attribute::fill_rule, svgpp::tag::value::evenodd) {
        style().nonzero_fill_rule_ = false;
    }

    void set(svgpp::tag::attribute::stroke_linecap, svgpp::tag::value::butt) {}

    void set(svgpp::tag::attribute::stroke_linecap, svgpp::tag::value::round) {}

    void set(svgpp::tag::attribute::stroke_linecap, svgpp::tag::value::square) {
    }
    void set(svgpp::tag::attribute::stroke_linejoin, svgpp::tag::value::miter) {
    }
    void set(svgpp::tag::attribute::stroke_linejoin, svgpp::tag::value::round) {
    }

    void set(svgpp::tag::attribute::stroke_linejoin, svgpp::tag::value::bevel) {
    }

    void set(svgpp::tag::attribute::stroke_miterlimit, double val) {
        style().miterlimit_ = val;
    }

    void set(svgpp::tag::attribute::stroke_dasharray, svgpp::tag::value::none) {
        style().stroke_dasharray_.clear();
    }

    template <class Range>
    void set(svgpp::tag::attribute::stroke_dasharray, Range const &range) {
        style().stroke_dasharray_.assign(boost::begin(range),
                                         boost::end(range));
    }

    void set(svgpp::tag::attribute::stroke_dashoffset, double val) {
        style().stroke_dashoffset_ = val;
    }

    template <class IRI>
    void set(svgpp::tag::attribute::mask, IRI const &) {
        throw std::runtime_error("Non-local references aren't supported");
    }

    template <class IRI>
    void set(svgpp::tag::attribute::mask, svgpp::tag::iri_fragment,
             IRI const &fragment) {
        style().mask_fragment_ =
            std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::mask, svgpp::tag::value::none val) {
        style().mask_fragment_.reset();
    }

    void set(svgpp::tag::attribute::mask, svgpp::tag::value::inherit val) {
        style().mask_fragment_ = parentStyle_.mask_fragment_;
    }

    template <class IRI>
    void set(svgpp::tag::attribute::clip_path, IRI const &) {
        throw std::runtime_error("Non-local references aren't supported");
    }

    template <class IRI>
    void set(svgpp::tag::attribute::clip_path, svgpp::tag::iri_fragment,
             IRI const &fragment) {
        style().clip_path_fragment_ =
            std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::clip_path, svgpp::tag::value::none val) {
        style().clip_path_fragment_.reset();
    }

    void set(svgpp::tag::attribute::clip_path, svgpp::tag::value::inherit val) {
        style().clip_path_fragment_ = parentStyle_.clip_path_fragment_;
    }

    Style &style() { return style_; }
    Style const &style() const { return style_; }

    template <class IRI>
    void set(svgpp::tag::attribute::marker_start, IRI const &) {
        std::cout << "Non-local references aren't supported\n"; // Not error
        style().marker_start_.reset();
    }

    template <class IRI>
    void set(svgpp::tag::attribute::marker_start, svgpp::tag::iri_fragment,
             IRI const &fragment) {
        style().marker_start_ =
            std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::marker_start, svgpp::tag::value::none) {
        style().marker_start_.reset();
    }

    template <class IRI>
    void set(svgpp::tag::attribute::marker_mid, IRI const &) {
        std::cout << "Non-local references aren't supported\n"; // Not error
        style().marker_mid_.reset();
    }

    template <class IRI>
    void set(svgpp::tag::attribute::marker_mid, svgpp::tag::iri_fragment,
             IRI const &fragment) {
        style().marker_mid_ =
            std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::marker_mid, svgpp::tag::value::none) {
        style().marker_mid_.reset();
    }

    template <class IRI>
    void set(svgpp::tag::attribute::marker_end, IRI const &) {
        std::cout << "Non-local references aren't supported\n"; // Not error
        style().marker_end_.reset();
    }

    template <class IRI>
    void set(svgpp::tag::attribute::marker_end, svgpp::tag::iri_fragment,
             IRI const &fragment) {
        style().marker_end_ =
            std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::marker_end, svgpp::tag::value::none) {
        style().marker_end_.reset();
    }

    template <class IRI>
    void set(svgpp::tag::attribute::marker, IRI const &) {
        std::cout << "Non-local references aren't supported\n"; // Not error
        style().marker_start_.reset();
        style().marker_mid_.reset();
        style().marker_end_.reset();
    }

    template <class IRI>
    void set(svgpp::tag::attribute::marker, svgpp::tag::iri_fragment,
             IRI const &fragment) {
        std::string iri(boost::begin(fragment), boost::end(fragment));
        style().marker_start_ = iri;
        style().marker_mid_ = iri;
        style().marker_end_ = iri;
    }

    void set(svgpp::tag::attribute::marker, svgpp::tag::value::none) {
        style().marker_start_.reset();
        style().marker_mid_.reset();
        style().marker_end_.reset();
    }

    template <class IRI>
    void set(svgpp::tag::attribute::filter, IRI const &) {
        throw std::runtime_error("Non-local references aren't supported");
    }

    template <class IRI>
    void set(svgpp::tag::attribute::filter, svgpp::tag::iri_fragment,
             IRI const &fragment) {
        style().filter_ =
            std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::filter, svgpp::tag::value::none val) {
        style().filter_.reset();
    }

    void set(svgpp::tag::attribute::filter, svgpp::tag::value::inherit) {
        style().filter_ = parentStyle_.filter_;
    }

    void set(svgpp::tag::attribute::overflow, svgpp::tag::value::inherit) {
        style().overflow_clip_ = parentStyle_.overflow_clip_;
    }

    void set(svgpp::tag::attribute::overflow, svgpp::tag::value::visible) {
        style().overflow_clip_ = false;
    }

    void set(svgpp::tag::attribute::overflow, svgpp::tag::value::auto_) {
        style().overflow_clip_ = false;
    }

    void set(svgpp::tag::attribute::overflow, svgpp::tag::value::hidden) {
        style().overflow_clip_ = true;
    }

    void set(svgpp::tag::attribute::overflow, svgpp::tag::value::scroll) {
        style().overflow_clip_ = true;
    }

    QString strokeColor() {
        if (auto *color_solid = boost::get<SolidPaint>(&style_.stroke_paint_)) {
            if (auto *ease_color = boost::get<color_t>(color_solid)) {
                QString label = "#" +
                                QString::number(ease_color->get<0>(), 16).rightJustified(2, '0') +
                                QString::number((int)ease_color->get<1>(), 16).rightJustified(2, '0') +
                                QString::number((int)ease_color->get<2>(), 16).rightJustified(2, '0');
                qInfo() << "Stroke color" << label;
                return label;
            }
        }
        return "Unknown color";
    }

  private:
    Style style_;
    NoninheritedStyle parentStyle_;
};
