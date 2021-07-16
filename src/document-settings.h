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

  QString machine_model;
  float width;
  float height;
  float dpi;
  bool use_af;
  bool use_diode;
  bool use_rotary;
  bool use_open_bottom;
};