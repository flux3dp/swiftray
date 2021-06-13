#ifndef LAYER_WIDGET_H
#define LAYER_WIDGET_H

#include <QWidget>
#include <canvas/layer.h>

namespace Ui {
class LayerListItem;
}

class LayerListItem : public QWidget {
    Q_OBJECT

  public:
    // explicit LayerListItem(QWidget *parent = nullptr);
    LayerListItem(QWidget *parent, LayerPtr &layer, bool active);
    void paintEvent(QPaintEvent *event) override;
    ~LayerListItem();
    LayerPtr layer_;

  private:
    void createIcon();
    void loadStyles();
    void registerEvents();

    Ui::LayerListItem *ui;
    bool active_;
};

#endif // LAYER_WIDGET_H
