#ifndef PRESETMANAGER_H
#define PRESETMANAGER_H

#include <QDialog>
#include <settings/preset-settings.h>

namespace Ui {
  class PresetManager;
}

class LayerParamsPanel;

class PresetManager : public QDialog {
Q_OBJECT

public:
  explicit PresetManager(QWidget *parent);

  ~PresetManager();

  void loadStyles();

  void loadSettings();

  void registerEvents();

  void showPreset();

  void updatePresetData();

  void save();

private:
  Ui::PresetManager *ui;
  LayerParamsPanel *layer_panel_;
};

#endif // PRESETMANAGER_H
