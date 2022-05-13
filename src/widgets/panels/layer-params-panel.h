#ifndef LAYER_PARAMS_PANEL_H
#define LAYER_PARAMS_PANEL_H

#include <QFrame>
#include <command.h>
#include <layer.h>
#include <settings/preset-settings.h>
#include <widgets/base-container.h>
#include <windows/preset-manager.h>

class MainWindow;

namespace Ui {
  class LayerParamsPanel;
}

class LayerParamsPanel : public QFrame, BaseContainer {
Q_OBJECT

public:
  explicit LayerParamsPanel(QWidget *parent, MainWindow *main_window);

  ~LayerParamsPanel();

public slots:

  void updateLayer(Layer *layer);

private:
  void loadStyles() override;

  void loadSettings() override;

  void registerEvents() override;

  void setToCustom();

  void updateMovingComboBox();

  Ui::LayerParamsPanel *ui;
  Layer *layer_;
  MainWindow *main_window_;
  PresetManager *preset_manager_;
  PresetSettings *preset_settings_ = &PresetSettings::getInstance();
  int preset_previous_index_;
};

#endif // LAYER_PARAMS_PANEL_H
