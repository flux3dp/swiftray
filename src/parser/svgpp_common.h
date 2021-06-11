#ifndef SVGCOMMON_H
#define SVGCOMMON_H

// Boost mpl parameters
#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_SET_SIZE 50
#define BOOST_PARAMETER_MAX_ARITY 15
// Following defines move parts of SVG++ code to svgpp_parser_impl.cpp file to
// boost compilation
#define SVGPP_USE_EXTERNAL_PATH_DATA_PARSER
#define SVGPP_USE_EXTERNAL_TRANSFORM_PARSER
#define SVGPP_USE_EXTERNAL_COLOR_PARSER
#define SVGPP_USE_EXTERNAL_PRESERVE_ASPECT_RATIO_PARSER
#define SVGPP_USE_EXTERNAL_PAINT_PARSER
#define SVGPP_USE_EXTERNAL_MISC_PARSER

// Include boost libraries in early stage
#include <boost/mpl/set.hpp>
#include <boost/mpl/set/set50.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/variant.hpp>

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <parser/svgpp_color_factory.h>
#include <svgpp/policy/xml/libxml2.hpp>
#include <svgpp/svgpp.hpp>

#include <string>

#include <QList>
#include <QString>
#include <canvas/layer.h>
#include <shape/path_shape.h>
#include <shape/shape.h>

using namespace svgpp;

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

extern QList<LayerPtr> *svgpp_layers;
extern QMap<QString, Layer*> *svgpp_layer_map;
extern LayerPtr svgpp_active_layer_;
bool svgpp_parse(QByteArray &data);
void svgpp_add_layer(LayerPtr layer);
void svgpp_add_shape(ShapePtr shape, QString layer_name);
void svgpp_set_active_layer(LayerPtr layer);
void svgpp_unset_active_layer();

#endif