#pragma once

#include <QDialog>
#include <settings/preset-settings.h>
#include <widgets/base-container.h>

namespace Ui {
  class PresetManager;
}

class LayerParamsPanel;

class PresetManager : public QDialog, BaseContainer {
Q_OBJECT

public:
  explicit PresetManager(QWidget *parent);

  ~PresetManager();

  void showPreset();

  void updatePresetData();

  void save();

private:

  void loadStyles() override;

  void loadSettings() override;

  void registerEvents() override;

  Ui::PresetManager *ui;
  LayerParamsPanel *layer_panel_;
};
