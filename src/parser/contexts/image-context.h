#include <QImage>
#include <QByteArray>
#include <parser/svgpp-common.h>
#include <shape/bitmap-shape.h>
#include <parser/contexts/base-context.h>

#pragma once

namespace Parser {

class ImageContext : public BaseContext {
public:
  ImageContext(BaseContext const &parent) : BaseContext(parent) {
    qInfo() << "Enter image";
    width_ = 0;
    height_ = 0;
    bitmap_ = nullptr;
  }

  boost::optional<double> const &width() const { return width_; }

  boost::optional<double> const &height() const { return height_; }

  using BaseContext::set;

  template<class IRI>
  void set(tag::attribute::xlink::href, tag::iri_fragment,
           IRI const &fragment) {
    qInfo() << "xlink::href" << fragment;
  }

  void set(tag::attribute::xlink::href, RangedChar fragment) {
    auto str = std::string(fragment.begin(), fragment.end());
    auto substr = std::string(fragment.begin() + 22, fragment.end());
    qInfo() << "xlink::href (from string)" << substr.size();
    QImage img = QImage::fromData(QByteArray::fromBase64(QString::fromStdString(substr).toUtf8()));
    qInfo() << "image size" << img.size();
    bitmap_ = std::make_shared<BitmapShape>(img);
  }

  void set(tag::attribute::x, double val) { x_ = val; }

  void set(tag::attribute::y, double val) { y_ = val; }

  void set(tag::attribute::width, double val) { width_ = val; }

  void set(tag::attribute::height, double val) { height_ = val; }

  void on_exit_element() {
    QString bitmap_layer_name("Bitmap");
    BitmapShape *new_shape = (BitmapShape *) bitmap_.get();
    if (width_ == 0) width_ = new_shape->pixmap()->width();
    if (height_ == 0) height_ = new_shape->pixmap()->height();
    new_shape->setTransform(
         qtransform() * QTransform().translate(x_, y_).scale(
              width_ / new_shape->pixmap()->width(),
              height_ / new_shape->pixmap()->height())

    );
    svgpp_add_shape(bitmap_, bitmap_layer_name);
  }

  std::string type() {
    return "image";
  }

private:
  std::string fragment_id_;
  double x_, y_;
  double width_, height_;
  shared_ptr<Shape> bitmap_;
};

}