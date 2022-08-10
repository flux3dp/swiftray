#pragma once

#include <QFont>
#include <shape/path-shape.h>
#include <QList>

class TextShape : public PathShape {
public:
  TextShape() noexcept;

  TextShape(QString text, QFont font, double line_height);

  void paint(QPainter *painter) const override;

  Shape::Type type() const override;

  std::shared_ptr<Shape> clone() const override;

  // Getters

  QString text();

  const QFont &font() const;

  float lineHeight() const;

  void makeCursorRect(int start_cursor, int end_cursor);

  int calculateCursor(QPointF point);

  bool isEditing() const;

  // Setters

  void setEditing(bool editing);

  void setText(QString text);

  void setFont(const QFont &font);

  void setLineHeight(float line_height);

  friend class DocumentSerializer;

private:
  bool editing_;
  float line_height_;
  QStringList lines_;
  QFont font_;
  QRectF cursor_rect_;
  QList<QRectF> select_box_;

  void makePath();
};

typedef std::shared_ptr<TextShape *> TextShapePtr;