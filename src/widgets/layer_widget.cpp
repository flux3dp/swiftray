#include <QDebug>
#include <QStyleOption>
#include <widgets/layer_widget.h>
#include "ui_layer_widget.h"

/*LayerWidget::LayerWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LayerWidget) {
    ui->setupUi(this);
}*/

LayerWidget::LayerWidget(QWidget *parent, LayerPtr &layer, bool active) :
    QWidget(parent),
    ui(new Ui::LayerWidget),
    layer_(layer),
    active_(active) {
    ui->setupUi(this);
    QPixmap pix(100, 100);
    pix.fill(QColor::fromRgba64(0, 0, 0, 0));
    QPainter paint(&pix);
    paint.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(QColor(255, 255, 255, 255), 5);
    paint.setPen(pen);
    paint.setBrush(QBrush(layer->color()));
    paint.drawRoundedRect(QRectF(30, 30, 40, 40), 10, 10);
    paint.end();
    ui->labelIcon->setPixmap(pix);
    ui->labelName->setText(layer->name);
    active_ = active;
    if (active_) {
        ui->layerWidgetFrame->setStyleSheet("#layerWidgetFrame { background-color: #0091ff; }");
        ui->labelName->setStyleSheet("color: white;");
    }
}

void LayerWidget::paintEvent(QPaintEvent* event) {
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
    QWidget::paintEvent(event);
}

LayerWidget::~LayerWidget() {
    delete ui;
}
