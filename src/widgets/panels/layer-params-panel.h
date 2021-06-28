#ifndef LAYER_PARAMS_PANEL_H
#define LAYER_PARAMS_PANEL_H

#include <QFrame>
#include <layer.h>
#include <settings/preset-settings.h>
#include "windows/preset-manager.h"

namespace Ui {
  class LayerParamsPanel;
}

class LayerParamsPanel : public QFrame {
Q_OBJECT

public:
  explicit LayerParamsPanel(QWidget *parent, Canvas *canvas);

  ~LayerParamsPanel();

public slots:

  void updateLayer(Layer *layer);

private:
  void loadStyles();

  void loadSettings();

  void registerEvents();

  Ui::LayerParamsPanel *ui;
  Layer *layer_;
  Canvas *canvas_;
  PresetManager *preset_manager_;
  int preset_previous_index_;
};

#endif // LAYER_PARAMS_PANEL_H
