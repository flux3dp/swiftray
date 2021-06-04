#include <QFont>
#include <shape/path_shape.h>

#ifndef TEXTSHAPE_H
#define TEXTSHAPE_H


class TextShape : public PathShape {
    public:
        TextShape() noexcept;
        TextShape(QString text, QFont font);
        Shape::Type type() const override;
        
        void paint(QPainter* painter) const override;
        QString text();
        QFont font();
        float lineHeight();

        void setText(QString text);
        void setFont(QFont font);
        void setLineHeight(float line_height);
        void makeCursorRect(int cursor);
        bool editing_;
        
    private:
        float line_height_;
        QStringList lines_;
        QFont font_;
        QRectF cursor_rect_;

        void makePath();
};

#endif // TEXTSHAPE_H
