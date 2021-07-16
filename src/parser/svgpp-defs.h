#pragma once

#include <parser/svgpp-common.h>
#include <layer.h>
#include <shape/path-shape.h>
#include <shape/shape.h>
#include <parser/css/svg-style-selector.h>

namespace Parser {

  typedef boost::variant<tag::value::none, tag::value::currentColor, color_t>
       SolidPaint;
  typedef boost::iterator_range<const char *> RangedChar;

  struct IRIPaint {
    IRIPaint(std::string const &fragment,
             boost::optional<SolidPaint> const &fallback =
             boost::optional<SolidPaint>()) {
      fragment_ = fragment;
      fallback_ = fallback;
    }

    std::string fragment_;
    boost::optional<SolidPaint> fallback_;
  };

  typedef boost::variant<SolidPaint, IRIPaint> SVGPPPaint;

  extern SVGStyleSelector *svgpp_style_selector;

}