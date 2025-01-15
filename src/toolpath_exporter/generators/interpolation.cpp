// This is modified from scipy
// https://github.com/scipy/scipy/blob/v1.10.1/scipy/interpolate/interpnd.pyx
// https://github.com/scipy/scipy/blob/v1.10.1/scipy/spatial/_qhull.pyx

#pragma once

#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMutex>
#include <QPainter>
#include <QProgressDialog>
#include <QVector2D>
#include <QVector3D>
#include <bitset>
#include <cmath>
#include <opencv2/flann.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

static QString get_key(double x, double y) {
  return QString("%1,%2").arg(x).arg(y);
}

class Triangle {
 public:
  std::vector<cv::Point2f> points;
  int point_indices[3];
  // indices for vertex in neighboring triangulation; -1 means no neighbor
  // e.g. triangle_neighbors[0], point_indices[1], point_indices[2] is another triangulation
  int triangle_neighbors[3] = {-1, -1, -1}; 

  Triangle(cv::Vec6f t,
           QMap<QString, int>* index_map,
           QList<QSet<int>>* vertex_neighbors_set) {
    for (int i = 0; i < 3; i++) {
      cv::Point2f p(t[2 * i], t[2 * i + 1]);
      this->points.push_back(p);
      QString key = get_key(p.x, p.y);
      int index = index_map->value(key);
      this->point_indices[i] = index;
    }
    for (int i = 0; i < 2; i++) {
      for (int j = i + 1; j < 3; j++) {
        (*vertex_neighbors_set)[this->point_indices[i]].insert(this->point_indices[j]);
        (*vertex_neighbors_set)[this->point_indices[j]].insert(this->point_indices[i]);
      }
    }
  }

  void update_neighbors(QList<QSet<int>>* vertex_neighbors_set) {
    for (int i = 0; i < 2; i++) {
      for (int j = i + 1; j < 3; j++) {
        int k = 3 - i - j;
        QSet<int> common((*vertex_neighbors_set)[this->point_indices[i]]);
        common.intersect((*vertex_neighbors_set)[this->point_indices[j]]);
        common.remove(this->point_indices[k]);
        if (!common.isEmpty()) {
          this->triangle_neighbors[k] = common.values()[0];
        }
      }
    }
  }

  bool contain_point(cv::Point2f p) {
    return cv::pointPolygonTest(this->points, p, false) >= 0;
  }
};

class CloughTocher2DInterpolator {
 public:
  int maxiter = 400;
  double tol = 1e-6;

  int min_x;
  int min_y;
  int max_x;
  int max_y;
  std::vector<cv::Point2f> points;
  std::vector<double> values;
  QMap<QString, int> index_map;
  cv::Subdiv2D subdiv;
  QList<QList<int>> vertex_neighbors;
  QList<double> grad;
  QList<Triangle> triangles;

  void set_bounding_box(int left, int top, int right, int bottom) {
    min_x = left;
    min_y = top;
    max_x = right;
    max_y = bottom;
  }

  void add_point(double x, double y, double z) {
    // Note: subdiv throws an error if some points are outside the bounding box
    // Update bounding box to ensure that all points are inside
    min_x = qMin(min_x, int(x));
    min_y = qMin(min_y, int(y));
    max_x = qMax(max_x, int(x));
    max_y = qMax(max_y, int(y));
    cv::Point2f p(x, y);
    points.push_back(p);
    values.push_back(z);
    index_map[get_key(x, y)] = points.size() - 1;
  }

  void setup() {
    std::vector<cv::Vec6f> triangleList;
    QList<QSet<int>> vertex_neighbors_sets(points.size());
    subdiv.initDelaunay(cv::Rect(min_x - 1, min_y - 1, max_x - min_x + 2, max_y - min_y + 2));
    subdiv.insert(points);
    subdiv.getTriangleList(triangleList);
    for (const auto t : triangleList) {
      triangles.push_back(Triangle(t, &index_map, &vertex_neighbors_sets));
    }
    for (auto t : triangles) {
      t.update_neighbors(&vertex_neighbors_sets);
    }
    for (const auto vertex_neighbors_set : vertex_neighbors_sets) {
      vertex_neighbors.push_back(vertex_neighbors_set.values());
    }
    grad.resize(points.size() * 2);
    bool converge = _estimate_gradients_2d_global(&grad);
    if (!converge) {
      qWarning() << "Gradient estimation did not converge, the results may be inaccurate";
    }
  }

  double do_evaluate(double x, double y) {
    double c[3];
    double f[3];
    double df[6];
    cv::Point2f p(x, y);
    for (auto t : triangles) {
      if (t.contain_point(p)) {
        get_barycentric_coordinate(p, t.points, &c[0]);
        for (int j = 0; j < 3; j++) {
          int ipoint = t.point_indices[j];
          f[j] = values[ipoint];
          df[2 * j] = grad[2 * ipoint];
          df[2 * j + 1] = grad[2 * ipoint + 1];
        }
        return _clough_tocher_2d_single(t, c, f, df);
      }
    }
    // outside of triangulation
    return std::nanf("");
  }

 private:
  void get_barycentric_coordinate(cv::Point2f p,
                                  std::vector<cv::Point2f> v,
                                  double* b) {
    cv::Point2f v0 = v[1] - v[0], v1 = v[2] - v[0], v2 = p - v[0];
    double d00 = v0.dot(v0);
    double d01 = v0.dot(v1);
    double d11 = v1.dot(v1);
    double d20 = v2.dot(v0);
    double d21 = v2.dot(v1);
    double denom = d00 * d11 - d01 * d01;
    b[1] = (d11 * d20 - d01 * d21) / denom;
    b[2] = (d00 * d21 - d01 * d20) / denom;
    b[0] = 1.0 - b[1] - b[2];
  }

  /**
   * Estimate gradients of a function at the vertices of a 2d triangulation.
   * @param y: output, shape (npoints, 2) Derivatives [F_x, F_y] at the vertices
   * @return: true if converged, false if maxiter reached without convergence
   */
  bool _estimate_gradients_2d_global(QList<double>* y) {
    double Q[2 * 2] = {0};
    double s[2] = {0};
    double r[2];
    double err;

    int len = points.size();
    int len2 = 2 * len;

    // initialize
    for (int ipoint = 0; ipoint < len2; ipoint++) {
      (*y)[ipoint] = 0.0;
    }

    // Gauss-Seidel
    for (int iiter = 0; iiter < maxiter; iiter++) {
      err = 0;
      for (int ipoint = 0; ipoint < len; ipoint++) {
        std::memset(Q, 0, sizeof(Q));
        std::memset(s, 0, sizeof(s));
        // walk over neighbours of given point
        int neighbor_cnt = vertex_neighbors[ipoint].size();
        for (int jpoint2 = 0; jpoint2 < neighbor_cnt; jpoint2++) {
          int ipoint2 = vertex_neighbors[ipoint][jpoint2];
          // edge
          double ex = points[ipoint2].x - points[ipoint].x;
          double ey = points[ipoint2].y - points[ipoint].y;
          double L = sqrt(ex * ex + ey * ey);
          double L3 = pow(L, 3);
          // data at vertices (Z height)
          double f1 = values[ipoint];
          double f2 = values[ipoint2];
          // scaled gradient projections on the edge
          double df2 = -ex * (*y)[2 * ipoint2 + 0] - ey * (*y)[2 * ipoint2 + 1];
          // edge sum
          Q[0] += 4 * ex * ex / L3;
          Q[1] += 4 * ex * ey / L3;
          Q[3] += 4 * ey * ey / L3;
          s[0] += (6 * (f1 - f2) - 2 * df2) * ex / L3;
          s[1] += (6 * (f1 - f2) - 2 * df2) * ey / L3;
        }
        Q[2] = Q[1];
        // solve
        double det = Q[0] * Q[3] - Q[1] * Q[2];
        r[0] = (Q[3] * s[0] - Q[1] * s[1]) / det;
        r[1] = (-Q[2] * s[0] + Q[0] * s[1]) / det;
        double change = qMax(fabs((*y)[2 * ipoint + 0] + r[0]),
                             fabs((*y)[2 * ipoint + 1] + r[1]));
        (*y)[2 * ipoint + 0] = -r[0];
        (*y)[2 * ipoint + 1] = -r[1];
        // relative/absolute error
        change /= qMax(1.0, qMax(fabs(r[0]), fabs(r[1])));
        err = qMax(err, change);
      }
      if (err < tol) {
        return true;
      }
    }
    // Didn't converge before maxiter
    return false;
  }

  /**
   * Evaluate Clough-Tocher interpolant on a 2D triangle.
   * @param t: triangle to evaluate on
   * @param c: barycentric coordinates of the point on the triangle
   * @param f: function values at vertices (z values)
   * @param df: gradient values at vertices
   * @return: value of the interpolant at the given point
   */
  double _clough_tocher_2d_single(Triangle t,
                                  double* b,
                                  double* f,
                                  double* df) {
    double c3000, c0300, c0030, c0003, c2100, c2010, c2001, c0210, c0201, c0021,
        c1200, c1020, c1002, c0120, c0102, c0012, c1101, c1011, c0111;
    double f1, f2, f3, df12, df13, df21, df23, df31, df32;
    double g[3];
    double e12x, e12y, e23x, e23y, e31x, e31y;
    double w;
    double minval;
    double b1, b2, b3, b4;
    int k, ipoint;
    double c[3];
    double y[2];

    e12x = t.points[1].x - t.points[0].x;
    e12y = t.points[1].y - t.points[0].y;

    e23x = t.points[2].x - t.points[1].x;
    e23y = t.points[2].y - t.points[1].y;

    e31x = t.points[0].x - t.points[2].x;
    e31y = t.points[0].y - t.points[2].y;

    f1 = f[0];
    f2 = f[1];
    f3 = f[2];

    df12 = +(df[2 * 0 + 0] * e12x + df[2 * 0 + 1] * e12y);
    df21 = -(df[2 * 1 + 0] * e12x + df[2 * 1 + 1] * e12y);
    df23 = +(df[2 * 1 + 0] * e23x + df[2 * 1 + 1] * e23y);
    df32 = -(df[2 * 2 + 0] * e23x + df[2 * 2 + 1] * e23y);
    df31 = +(df[2 * 2 + 0] * e31x + df[2 * 2 + 1] * e31y);
    df13 = -(df[2 * 0 + 0] * e31x + df[2 * 0 + 1] * e31y);

    c3000 = f1;
    c2100 = (df12 + 3 * c3000) / 3;
    c2010 = (df13 + 3 * c3000) / 3;
    c0300 = f2;
    c1200 = (df21 + 3 * c0300) / 3;
    c0210 = (df23 + 3 * c0300) / 3;
    c0030 = f3;
    c1020 = (df31 + 3 * c0030) / 3;
    c0120 = (df32 + 3 * c0030) / 3;

    c2001 = (c2100 + c2010 + c3000) / 3;
    c0201 = (c1200 + c0300 + c0210) / 3;
    c0021 = (c1020 + c0120 + c0030) / 3;

    for (k = 0; k < 3; k++) {
      ipoint = t.triangle_neighbors[k];
      if (ipoint == -1) {
        // No neighbour.
        // Compute derivative to the centroid direction (e_12 + e_13)/2.
        g[k] = -1. / 2;
        continue;
      }

      // Centroid of the neighbour, in our local barycentric coordinates
      y[0] = (t.points[0].x + t.points[1].x + t.points[2].x - t.points[k].x +
              points[ipoint].x) /
             3;
      y[1] = (t.points[0].y + t.points[1].y + t.points[2].y - t.points[k].y +
              points[ipoint].y) /
             3;
      get_barycentric_coordinate(cv::Point2f(y[0], y[1]), t.points, c);

      switch (k) {
        case 0:
          g[k] = (2 * c[2] + c[1] - 1) / (2 - 3 * c[2] - 3 * c[1]);
          break;
        case 1:
          g[k] = (2 * c[0] + c[2] - 1) / (2 - 3 * c[0] - 3 * c[2]);
          break;
        case 2:
        default:
          g[k] = (2 * c[1] + c[0] - 1) / (2 - 3 * c[1] - 3 * c[0]);
          break;
      }
    }
    c0111 = (g[0] * (-c0300 + 3 * c0210 - 3 * c0120 + c0030) +
             (-c0300 + 2 * c0210 - c0120 + c0021 + c0201)) /
            2;
    c1011 = (g[1] * (-c0030 + 3 * c1020 - 3 * c2010 + c3000) +
             (-c0030 + 2 * c1020 - c2010 + c2001 + c0021)) /
            2;
    c1101 = (g[2] * (-c3000 + 3 * c2100 - 3 * c1200 + c0300) +
             (-c3000 + 2 * c2100 - c1200 + c2001 + c0201)) /
            2;

    c1002 = (c1101 + c1011 + c2001) / 3;
    c0102 = (c1101 + c0111 + c0201) / 3;
    c0012 = (c1011 + c0111 + c0021) / 3;

    c0003 = (c1002 + c0102 + c0012) / 3;

    // extended barycentric coordinates
    minval = b[0];
    for (k = 0; k < 3; k++) {
      if (b[k] < minval)
        minval = b[k];
    }

    b1 = b[0] - minval;
    b2 = b[1] - minval;
    b3 = b[2] - minval;
    b4 = 3 * minval;

    // evaluate the polynomial -- the stupid and ugly way to do it,
    // one of the 4 coordinates is in fact zero
    w = (pow(b1, 3) * c3000 + 3 * pow(b1, 2) * b2 * c2100 +
         3 * pow(b1, 2) * b3 * c2010 + 3 * pow(b1, 2) * b4 * c2001 +
         3 * b1 * pow(b2, 2) * c1200 + 6 * b1 * b2 * b4 * c1101 +
         3 * b1 * pow(b3, 2) * c1020 + 6 * b1 * b3 * b4 * c1011 +
         3 * b1 * pow(b4, 2) * c1002 + pow(b2, 3) * c0300 +
         3 * pow(b2, 2) * b3 * c0210 + 3 * pow(b2, 2) * b4 * c0201 +
         3 * b2 * pow(b3, 2) * c0120 + 6 * b2 * b3 * b4 * c0111 +
         3 * b2 * pow(b4, 2) * c0102 + pow(b3, 3) * c0030 +
         3 * pow(b3, 2) * b4 * c0021 + 3 * b3 * pow(b4, 2) * c0012 +
         pow(b4, 3) * c0003);
    return w;
  }
};
