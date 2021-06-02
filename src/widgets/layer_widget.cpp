#include <widgets/layer_widget.h>
#include "ui_layer_widget.h"

/*LayerWidget::LayerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LayerWidget) {
    ui->setupUi(this);
}*/

LayerWidget::LayerWidget(QWidget *parent, Layer &layer) :
    QWidget(parent),
    ui(new Ui::LayerWidget),
    layer_ {layer} {
    ui->setupUi(this);
    QPixmap pix(96, 96);
    pix.fill(QColor::fromRgba64(0, 0, 0, 0));
    QPainter paint(&pix);
    paint.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(QColor(0, 0, 0, 255), 5);
    paint.setPen(pen);
    paint.setBrush(QBrush(layer.color()));
    paint.drawRoundedRect(QRectF(20, 20, 56, 56), 10, 10);
    paint.end();
    ui->labelIcon->setPixmap(pix);
    ui->labelName->setText(layer.name);
}

LayerWidget::~LayerWidget() {
    delete ui;
}
