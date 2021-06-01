#include <widgets/layer_widget.h>
#include <widgets/ui_layer_widget.h>

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
    QPixmap pix(128, 96);
    pix.fill(QColor::fromRgba64(0, 0, 0, 0));
    QPainter paint(&pix);
    QPen pen(QColor(0, 0, 0, 255), 5);
    paint.setPen(pen);
    paint.fillRect(QRectF(12, 12, 72, 72), QBrush(layer.color()));
    paint.drawRoundedRect(QRectF(10, 10, 76, 76), 5, 5);
    paint.end();
    ui->labelIcon->setPixmap(pix);
    ui->labelName->setText(layer.name);
}

LayerWidget::~LayerWidget() {
    delete ui;
}
