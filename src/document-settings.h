#pragma once

class DocumentSettings {
public:
  DocumentSettings() :
       machine_model("beamo"),
       width(3000),
       height(2000),
       dpi(250),
       use_af(false),
       use_diode(false),
       use_rotary(false),
       use_open_bottom(false) {}

  // NOTE: Use an approx. value for DPI-DPMM conversion (25.4 -> 25 inch/mm)
  float dpmm() { return dpi / 25; }

  QString machine_model; // TBD
  float width;  // NOTE: store value but not used?
  float height; // NOTE: store value but not used?
  float dpi;
  bool use_af;
  bool use_diode;
  bool use_rotary;
  bool use_open_bottom;
};
