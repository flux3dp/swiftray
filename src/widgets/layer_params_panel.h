#include <canvas/layer.h>
#ifndef LAYER_PARAMS_PANEL_H
#define LAYER_PARAMS_PANEL_H

#include <QFrame>

namespace Ui {
class LayerParamsPanel;
}

class LayerParamsPanel : public QFrame
{
    Q_OBJECT

public:
    explicit LayerParamsPanel(QWidget *parent = nullptr);
    ~LayerParamsPanel();
public slots:
    void updateLayer(LayerPtr layer);

private:
    void loadStyles();
    void registerEvents();

    Ui::LayerParamsPanel *ui;
    LayerPtr layer_;
};

#endif // LAYER_PARAMS_PANEL_H
