#include <QDebug>
#include <parser/base_context.h>
#include <parser/use_context.h>

#pragma once

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
