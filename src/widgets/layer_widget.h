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
        //explicit LayerWidget(QWidget *parent = nullptr);
        LayerWidget(QWidget *parent, Layer &layer, bool active);
        void paintEvent(QPaintEvent* event) override;
        ~LayerWidget();

    private:
        Ui::LayerWidget *ui;
        Layer &layer_;
        bool active_;
};

#endif // LAYER_WIDGET_H
