#ifndef LAYER_WIDGET_H
#define LAYER_WIDGET_H

#include <QWidget>
#include <canvas/layer.h>

namespace Ui {
class LayerWidget;
}

class LayerWidget : public QWidget {
    Q_OBJECT

  public:
    // explicit LayerWidget(QWidget *parent = nullptr);
    LayerWidget(QWidget *parent, LayerPtr &layer, bool active);
    void paintEvent(QPaintEvent *event) override;
    ~LayerWidget();
    LayerPtr &layer_;

  private:
    Ui::LayerWidget *ui;
    bool active_;
};

#endif // LAYER_WIDGET_H
