#include <QDebug>
#include <QDateTime>
#include <shape/text_shape.h>

TextShape::TextShape() noexcept: PathShape() {
    line_height_ = 1.2;
}

TextShape::TextShape(QString text, QFont font): PathShape() {
    line_height_ = 1.2;
    lines_ = text.split("\n");
    font_ = font;
    makePath();
}

Shape::Type TextShape::type() const {
    return Shape::Type::Text;
}

QString TextShape::text() {
    return lines_.join("\n");
}

QFont TextShape::font() {
    return font_;
}

bool TextShape::isEditing() {
    return editing_;
}

float TextShape::lineHeight() {
    return line_height_;
}

void TextShape::setLineHeight(float line_height) {
    line_height_ = line_height;
}

void TextShape::setText(QString text) {
    lines_ = text.split("\n");
    if (lines_.length() == 0) lines_ << "";
    makePath();
}

void TextShape::setFont(QFont font) {
    font_ = font;
    makePath();
}

void TextShape::setEditing(bool editing) {
    editing_ = editing;
}

void TextShape::makePath() {
    QPainterPath path;
    for(int i = 0; i < lines_.length(); i++) {
        QString &line = lines_[i];
        path.addText(QPointF(0, i * line_height_ * font_.pointSizeF()), font_, line);
    };
    setPath(path);
}

void TextShape::makeCursorRect(int cursor) {
    int current_pos = 0;
    for(int i = 0; i < lines_.length(); i++) {
        QString &line = lines_[i];
        int cursor_offset = cursor - current_pos;
        if (cursor_offset >= 0 && cursor_offset <= line.length()) {
            // TODO :: remove line.length() < cursor_offset
            QString test_string = (cursor_offset == 0 || line.length() < cursor_offset) ? "A" : 
                                  line.chopped(line.length() - cursor_offset);

            QPainterPath line_path;
            line_path.addText(QPointF(0, i * line_height_ * font_.pointSizeF()), 
                              font_, 
                              test_string);

            QRectF test_rect = line_path.boundingRect();
            if (cursor_offset == 0) {
                cursor_rect_ = QRectF(test_rect.topLeft(), QSizeF(5, test_rect.height()));
            } else {
                cursor_rect_ = QRectF(test_rect.topRight() + QPointF(5, 0), QSizeF(5, test_rect.height()));
            }
            break;
        }
        current_pos += lines_[i].length() + 1; // Add "\n"'s offset
    };
}

void TextShape::paint(QPainter* painter) const {
    painter->save();
    painter->setTransform(transform(), true);
    painter->setTransform(temp_transform_, true);
    if (editing_ && 
        QDateTime::currentMSecsSinceEpoch() % 1000 < 500) {
        // Show 500 msec and hide 500 msec
        painter->drawRect(cursor_rect_);
    }
    painter->restore();
    PathShape::paint(painter);
}
