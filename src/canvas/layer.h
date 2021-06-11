#include <shape/shape.h>

#ifndef LAYER_H
#define LAYER_H

class Layer {
  public:
    Layer();
    Layer(int new_layer_id);
    void paint(QPainter *painter, int counter) const;
    void addShape(ShapePtr shape);
    void removeShape(ShapePtr shape);
    void clear();
    shared_ptr<Layer> clone();
    QList<ShapePtr> &children();

    double repeat() const;
    double speed() const;
    double strength() const;
    QColor color() const;
    QString name() const;

    void setColor(QColor color);
    void setHeight(double height);
    void setName(const QString &name);
    void setSpeed(double speed);
    void setStrength(double strength);
    void setRepeat(double repeat);
    void setDiode(int diode_);
    void setZStep(double zstep);

  private:
    QColor color_;
    QString name_;
    QList<ShapePtr> children_;
    double strength_;
    double speed_;
    double zstep_;
    bool diode_;
    bool repeat_;
    bool height_;
};

typedef shared_ptr<Layer> LayerPtr;

#endif // LAYER_H
