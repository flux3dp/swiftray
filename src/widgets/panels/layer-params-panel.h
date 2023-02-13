#ifndef LAYER_PARAMS_PANEL_H
#define LAYER_PARAMS_PANEL_H

#include <QFrame>
#include <command.h>
#include <layer.h>
#include <widgets/base-container.h>
#include <QToolButton>

class MainWindow;

namespace Ui {
  class LayerParamsPanel;
}

class LayerParamsPanel : public QFrame, BaseContainer {
Q_OBJECT

public:
  explicit LayerParamsPanel(QWidget *parent, MainWindow *main_window);
  ~LayerParamsPanel();
  void setPresetIndex(int preset_index, int param_index);
  void setLayerParam(double strength, double speed, int repeat);
  void setLayerBacklash(double backlash);
  void setLayerParamLock(bool enable);

public Q_SLOTS:
  void updateLayer(Layer *layer);

Q_SIGNALS:
  void editParamIndex(int param_index);
  void wakeupPresetManager();
  void editLayerParam(double strength, double speed, int repeat);
  void editLayerBacklash(double backlash);
  void editLayerParamIndex(int param_index);

private:
  void loadStyles() override;
  void loadSettings() override;
  void registerEvents() override;
  void resizeEvent(QResizeEvent *) override;
  void setToCustom();
  void updateMovingComboBox();

  Ui::LayerParamsPanel *ui;
  Layer *layer_;
  MainWindow *main_window_;
  QToolButton *add_layer_btn_;
};

#endif // LAYER_PARAMS_PANEL_H
