#include <QDateTime>
#include <QDebug>
#include <shape/text-shape.h>

TextShape::TextShape() noexcept: PathShape() { line_height_ = 1.2; }

TextShape::TextShape(QString text, QFont font) : PathShape() {
  line_height_ = 1.2;
  lines_ = text.split("\n");
  font_ = font;
  makePath();
}

Shape::Type TextShape::type() const { return Shape::Type::Text; }

QString TextShape::text() { return lines_.join("\n"); }

const QFont &TextShape::font() const { return font_; }

bool TextShape::isEditing() const { return editing_; }

float TextShape::lineHeight() const { return line_height_; }

void TextShape::setLineHeight(float line_height) {
  line_height_ = line_height;
  makePath();
}

void TextShape::setText(QString text) {
  lines_ = text.split("\n");
  if (lines_.empty())
    lines_ << "";
  makePath();
}

void TextShape::setFont(const QFont &font) {
  font_ = font;
  makePath();
}

void TextShape::setEditing(bool editing) { editing_ = editing; }

void TextShape::makePath() {
  QPainterPath path;
  for (int i = 0; i < lines_.length(); i++) {
    QString &line = lines_[i];
    path.addText(QPointF(0, i * line_height_ * font_.pointSizeF()), font_,
                 line);
  };
  setPath(path);
}

void TextShape::makeCursorRect(int cursor) {
  int current_pos = 0;
  const qreal WIDTH_HEIGHT_RATIO = 0.03; // fixed ratio between width and height
  for (int i = 0; i < lines_.length(); i++) {
    QString &line = lines_[i];
    int cursor_offset = cursor - current_pos;
    if (cursor_offset >= 0 && cursor_offset <= line.length()) {
      QString test_string =
           (cursor_offset == 0 || line.length() < cursor_offset)
           ? "A"
           : line.chopped(line.length() - cursor_offset);

      QPainterPath line_path; // for calculating x pos of cursor rect
      line_path.addText(QPointF(0, i * line_height_ * font_.pointSizeF()),
                        font_, test_string);
      QPainterPath height_path; // always use "A" for calculating y_offset and height of cursor
      height_path.addText(QPointF(0, i * line_height_ * font_.pointSizeF()),
                        font_, "A");

      qreal cursor_height = height_path.boundingRect().height();
      qreal cursor_width = cursor_height * WIDTH_HEIGHT_RATIO;
      qreal cursor_x_pos = cursor_offset == 0 ? line_path.boundingRect().left() :
                                                line_path.boundingRect().right() + cursor_width;
      qreal cursor_y_pos = height_path.boundingRect().top();
      cursor_rect_ = QRectF{QPointF{cursor_x_pos, cursor_y_pos},
                              QSizeF{cursor_width, cursor_height}};
      break;
    }
    current_pos += lines_[i].length() + 1; // Add "\n"'s offset
  };
}

void TextShape::paint(QPainter *painter) const {
  painter->save();
  painter->setTransform(transform(), true);
  painter->setTransform(temp_transform_, true);
  if (editing_ && QDateTime::currentMSecsSinceEpoch() % 1000 < 500) {
    // Show 500 msec and hide 500 msec
    painter->fillRect(cursor_rect_, QColor{20, 20, 20, 150});
  }
  painter->restore();
  PathShape::paint(painter);
}

std::shared_ptr<Shape> TextShape::clone() const {
  std::shared_ptr<TextShape> shape = std::make_shared<TextShape>(*this);
  return shape;
}