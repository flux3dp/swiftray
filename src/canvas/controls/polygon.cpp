#include <QPainterPath>
#include <canvas/controls/polygon.h>
#include <shape/path-shape.h>
#include <canvas/canvas.h>
#include <QTransform>
#include <QPoint>


using namespace Controls;

Polygon::Polygon(Canvas *canvas) noexcept: CanvasControl(canvas) {
  num_side_ = kDefaultNumSide;
}

bool Polygon::isActive() {
  return canvas().mode() == Canvas::Mode::PolygonDrawing;
}

bool Polygon::mouseMoveEvent(QMouseEvent *e) {

  initial_vertex_ =  document().getCanvasCoord(e->pos());
  center_ = document().mousePressedCanvasCoord();
  updateVertices(center_, initial_vertex_);

  return true;
}

bool Polygon::mouseReleaseEvent(QMouseEvent *e) {

  if (polygon_.empty()) {
    exit();
    return true;
  }

  QPainterPath path;
  path.addPolygon(polygon_);
  ShapePtr new_polygon = make_shared<PathShape>(path);
  canvas().setMode(Canvas::Mode::Selecting);
  document().execute(
          Commands::AddShape(document().activeLayer(), new_polygon),
          Commands::Select(&document(), {new_polygon})
  );
  exit();
  return true;
}

void Polygon::paint(QPainter *painter) {
  QPen pen(document().activeLayer()->color(), 2, Qt::SolidLine);
  pen.setCosmetic(true);
  painter->setPen(pen);
  painter->drawConvexPolygon(polygon_);
}


bool Polygon::keyPressEvent(QKeyEvent *e) {
  if (e->key() == Qt::Key::Key_Escape) {
    exit();
    return true;
  }
  if (e->key() == Qt::Key::Key_Equal) {
    if (!polygon_.empty()) {
      num_side_ += 1;
      updateVertices(center_, initial_vertex_);
      return true;
    }
  }
  if (e->key() == Qt::Key::Key_Minus) {
    if (!polygon_.empty() && num_side_ > kMinimumNumSide) {
      num_side_ -= 1;
      updateVertices(center_, initial_vertex_);
      return true;
    }
  }

  return false;
}

void Polygon::exit() {
  num_side_ = kDefaultNumSide;
  polygon_ = QPolygonF();
  canvas().setMode(Canvas::Mode::Selecting);
}

void Polygon::updateVertices(const QPointF &center, const QPointF &start_vertex) {
  // Translate center to origin -> generate vertices by rotating initial vertex -> translate back center

  polygon_ = QPolygonF();

  QTransform translate_center = QTransform::fromTranslate( (-1) * center.x(), (-1) * center.y());
  QTransform rotate_transform;

  QPointF polygon_vertex = translate_center.map(start_vertex);
  polygon_ << polygon_vertex;
  for (auto i = 1; i <= num_side_; i++) {
    rotate_transform.reset();
    rotate_transform.rotate(qreal(360 * i) / num_side_);
    polygon_ << rotate_transform.map(polygon_vertex);
  }
  polygon_ = translate_center.inverted().map(polygon_);

  Q_ASSERT_X(polygon_.isClosed() == true, "Polygon", "Generated polygon must be closed");
}


bool Polygon::setNumSide(unsigned int numSide) {
  if (numSide >= kMinimumNumSide) {
    num_side_ = numSide;
    if (!polygon_.empty()) {
      updateVertices(center_, initial_vertex_);
    }
    return true;
  } else {
    return false;
  }
}