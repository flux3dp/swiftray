#pragma once

#include <QDialog>
#include <widgets/base-container.h>
#include <QListWidgetItem>

namespace Ui {
  class PresetManager;
}

class PresetManager : public QDialog, BaseContainer {
Q_OBJECT

public:
  explicit PresetManager(QWidget *parent, int preset_index);
  ~PresetManager();

Q_SIGNALS:
  void updateCurrentPresetIndex(int index);
  void updateCurrentIndex(int preset_index, int param_index);
  void savePresets();

private:
  void loadStyles() override;
  void loadSettings() override;
  void registerEvents() override;
  void save();
  void showPreset();
  void updatePresetData();

  Ui::PresetManager *ui;
  QListWidgetItem *current_param_;
};
