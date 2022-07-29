#include <QDateTime>
#include <QDebug>
#include <constants.h>
#include <shape/text-shape.h>

TextShape::TextShape() noexcept: PathShape() { line_height_ = LINE_HEIGHT; }

TextShape::TextShape(QString text, QFont font, double line_height) : PathShape() {
  line_height_ = line_height;
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
  QPainterPath infill{path};
  infill.setFillRule(Qt::WindingFill);
  path = path.united(infill);
  setPath(path);
}

void TextShape::makeCursorRect(int start_cursor, int end_cursor) {
  select_box_.clear();
  int current_pos = 0;
  const qreal WIDTH_HEIGHT_RATIO = 0.03; // fixed ratio between width and height
  for (int i = 0; i < lines_.length(); i++) {
    QString &line = lines_[i];
    int start_offset = start_cursor - current_pos;
    int cursor_offset = end_cursor - current_pos;
    if (start_cursor != end_cursor && current_pos + line.length() >= start_cursor) {
      QPainterPath height_path; // always use "A" for calculating y_offset and height of cursor
      height_path.addText(QPointF(0, i * line_height_ * font_.pointSizeF()),
                        font_, "Aj");
      if (cursor_offset >= 0 && cursor_offset <= line.length()) {
        if(start_offset >= 0) {
          QString select_string = line.chopped(line.length() - cursor_offset);
          select_string = select_string.right(select_string.length() - start_offset);
          if(line.length() == cursor_offset)
            select_string += "i";
          QString space_string = line.chopped(line.length() - start_offset);
          QPainterPath empty_path; // for calculating x pos of cursor rect
          empty_path.addText(QPointF(0, i * line_height_ * font_.pointSizeF()),
                            font_, space_string);
          QPainterPath line_path; // for calculating x pos of cursor rect
          line_path.addText(QPointF(0, i * line_height_ * font_.pointSizeF()),
                            font_, select_string);
          qreal select_x_pos = empty_path.boundingRect().right();
          qreal select_y_pos = height_path.boundingRect().top();
          qreal select_height = height_path.boundingRect().height();
          qreal select_width = line_path.boundingRect().right();
          select_box_.push_back(QRectF{QPointF{select_x_pos, select_y_pos},
                                      QSizeF{select_width, select_height}});
        }
        else {
          QString select_string = line.chopped(line.length() - cursor_offset);
          if(line.length() == cursor_offset)
            select_string += "i";
          QPainterPath line_path; // for calculating x pos of cursor rect
          line_path.addText(QPointF(0, i * line_height_ * font_.pointSizeF()),
                            font_, select_string);
          qreal select_x_pos = line_path.boundingRect().left();
          qreal select_y_pos = height_path.boundingRect().top();
          qreal select_height = height_path.boundingRect().height();
          qreal select_width = line_path.boundingRect().right();
          select_box_.push_back(QRectF{QPointF{select_x_pos, select_y_pos},
                                      QSizeF{select_width, select_height}});
        }
      }
      else if(start_offset >= 0) {
        QString space_string = line.chopped(line.length() - start_offset);
        QPainterPath empty_path; // for calculating x pos of cursor rect
        empty_path.addText(QPointF(0, i * line_height_ * font_.pointSizeF()),
                          font_, space_string);
        QPainterPath line_path; // for calculating width rect
        line_path.addText(QPointF(0, i * line_height_ * font_.pointSizeF()),
                          font_, line.right(line.length() - start_offset) + "i");
        qreal select_x_pos = empty_path.boundingRect().right();
        qreal select_y_pos = height_path.boundingRect().top();
        qreal select_height = height_path.boundingRect().height();
        qreal select_width = line_path.boundingRect().right();
        select_box_.push_back(QRectF{QPointF{select_x_pos, select_y_pos},
                                    QSizeF{select_width, select_height}});
      }
      else {
        QPainterPath line_path; // for calculating width rect
        line_path.addText(QPointF(0, i * line_height_ * font_.pointSizeF()),
                          font_, line + "i");
        qreal select_x_pos = line_path.boundingRect().left();
        qreal select_y_pos = height_path.boundingRect().top();
        qreal select_height = height_path.boundingRect().height();
        qreal select_width = line_path.boundingRect().right();
        select_box_.push_back(QRectF{QPointF{select_x_pos, select_y_pos},
                                    QSizeF{select_width, select_height}});
      }
    }
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

int TextShape::calculateCursor(QPointF point) {
  int current_pos = 0;
  const qreal WIDTH_HEIGHT_RATIO = 0.03; // fixed ratio between width and height
  for (int i = 0; i < lines_.length(); i++) {
    QString &line = lines_[i];

    QPainterPath height_path; // always use "A" for calculating y_offset and height of cursor
    height_path.addText(QPointF(0, i * line_height_ * font_.pointSizeF()),
                        font_, "A");
    qreal cursor_height = height_path.boundingRect().height();
    qreal cursor_y_pos = height_path.boundingRect().top();
    if(((point-pos()).y()) <= (cursor_y_pos + cursor_height) ||
      i == lines_.length()-1 ) {
      for(unsigned int cursor_offset = 1; cursor_offset <= line.length(); cursor_offset++) {
        current_pos++;
        QString test_string = line.chopped(line.length() - cursor_offset);
        QPainterPath line_path; // for calculating x pos of cursor rect
        line_path.addText(QPointF(0, i * line_height_ * font_.pointSizeF()),
                        font_, test_string);
        qreal cursor_x_pos = line_path.boundingRect().right();
        if((point-pos()).x() <= cursor_x_pos) {
          cursor_x_pos = line_path.boundingRect().left();
          if((point-pos()).x() <= cursor_x_pos) {
            current_pos--;
          }
          break;
        }
      }
      break;
    }
    current_pos += lines_[i].length() + 1; // Add "\n"'s offset
  };
  return current_pos;
}

void TextShape::paint(QPainter *painter) const {
  painter->save();
  painter->setTransform(transform(), true);
  painter->setTransform(temp_transform_, true);
  for (auto &select_rect : select_box_) {
    painter->fillRect(select_rect, QColor{20, 20, 20, 80});
  }
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
