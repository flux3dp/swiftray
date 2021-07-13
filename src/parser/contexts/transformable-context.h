#include <parser/svgpp-common.h>

#include <boost/math/constants/constants.hpp>
#include <boost/numeric/ublas/assignment.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix.hpp>

#include <QDebug>

#pragma once

namespace Parser {

namespace ublas = boost::numeric::ublas;
typedef ublas::matrix<double> matrix_t;

class TransformableContext {
public:
  QPointF getTransformedPos(double x, double y) {
    // Disable this
    return QPointF(x, y);
    matrix_t m = ublas::identity_matrix<double>(3, 1);
    m <<= x, y, 1;
    matrix_t npos = ublas::prod(transform_, m);
    return QPointF(npos(0, 0), npos(1, 0));
  }

  QTransform qtransform() {
    return QTransform(transform_(0, 0), transform_(1, 0), transform_(2, 0), transform_(0, 1), transform_(1, 1),
                      transform_(2, 1), transform_(0, 2), transform_(1, 2), transform_(2, 2));
  }

  void transform_matrix(const boost::array<double, 6> &matrix) {
    matrix_t m(3, 3);
    m <<= matrix[0], matrix[2], matrix[4], matrix[1], matrix[3], matrix[5], 0,
         0, 1;
    // TODO(warn: the logic here means transforms will be joined by svgpp, but it is not guaranteed by the library)
    transform_ = m; // should be "ublas::prod(transform_, m);"
  }

  void transform_translate(double tx, double ty) {
    matrix_t m = ublas::identity_matrix<double>(3, 3);
    m(0, 2) = tx;
    m(1, 2) = ty;
    transform_ = ublas::prod(transform_, m);
    qInfo() << "Transform translate";
  }

  void transform_translate(double tx) {
    transform_translate(tx, tx);
    qInfo() << "Transform translate";
  }

  void transform_scale(double sx, double sy) {
    matrix_t m = ublas::identity_matrix<double>(3, 3);
    m(0, 0) = sx;
    m(1, 1) = sy;
    transform_ = ublas::prod(transform_, m);
    // qInfo() << "Transform scale";
  }

  void transform_scale(double scale) {
    transform_scale(scale, scale);
    // qInfo() << "Transform scale";
  }

  void transform_rotate(double angle) {
    angle *= boost::math::constants::degree<double>();
    matrix_t m(3, 3);
    m <<= std::cos(angle), -std::sin(angle), 0, std::sin(angle),
         std::cos(angle), 0, 0, 0, 1;
    transform_ = ublas::prod(transform_, m);
    qInfo() << "Transform rotate";
  }

  matrix_t &transform() { return transform_; }

  matrix_t const &transform() const { return transform_; }

  matrix_t transform_;
};

}