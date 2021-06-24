#ifndef PRESETMANAGER_H
#define PRESETMANAGER_H

#include <QDialog>

namespace Ui {
  class PresetManager;
}

class PresetManager : public QDialog {
Q_OBJECT

public:
  explicit PresetManager(QWidget *parent = nullptr);

  ~PresetManager();

  void loadStyles();

  void loadSettings();

  void registerEvents();

private:
  Ui::PresetManager *ui;
};

#endif // PRESETMANAGER_H
