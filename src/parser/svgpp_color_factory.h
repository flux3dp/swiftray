#pragma once

#include <boost/preprocessor/tuple/elem.h>
#include <svgpp/factory/integer_color.h>

typedef boost::tuple<unsigned char, unsigned char, unsigned char> color_t;

struct ColorFactoryBase
{
  typedef color_t color_type;

  static color_type create(unsigned char r, unsigned char g, unsigned char b)
  {
    return color_t(r, g, b);
  }
};
typedef svgpp::factory::color::percentage_adapter<ColorFactoryBase> ColorFactory;