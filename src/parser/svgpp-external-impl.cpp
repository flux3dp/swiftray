#include <parser/svgpp-common.h>
#include <svgpp/parser/external_function/parse_all_impl.hpp>
#include <svgpp/factory/color.hpp>

/* Building external parsing function (meant to accelerate compiling process) */
SVGPP_PARSE_PATH_DATA_IMPL(const char *, double)

SVGPP_PARSE_TRANSFORM_IMPL(const char *, double)

SVGPP_PARSE_PAINT_IMPL(const char *, Parser::ColorFactory, svgpp::factory::icc_color::default_factory)

SVGPP_PARSE_COLOR_IMPL(const char *, Parser::ColorFactory, svgpp::factory::icc_color::default_factory)

SVGPP_PARSE_PRESERVE_ASPECT_RATIO_IMPL(const char *)

SVGPP_PARSE_MISC_IMPL(const char *, double)
