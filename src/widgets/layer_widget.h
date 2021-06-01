#ifndef LAYER_WIDGET_H
#define LAYER_WIDGET_H

#include <QWidget>
#include <src/canvas/layer.h>

namespace Ui {
    class LayerWidget;
}

class LayerWidget : public QWidget {
        Q_OBJECT

    public:
        //explicit LayerWidget(QWidget *parent = nullptr);
        LayerWidget(QWidget *parent, Layer &layer);
        ~LayerWidget();

    private:
        Ui::LayerWidget *ui;
        Layer &layer_;
};

#endif // LAYER_WIDGET_H
