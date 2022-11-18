#pragma once

#include <QToolButton>


QT_BEGIN_NAMESPACE
namespace Ui { class ColorPickerButton; }
QT_END_NAMESPACE

class ColorPickerButton : public QToolButton {
Q_OBJECT

public:
    explicit ColorPickerButton(QWidget *parent = nullptr);
    explicit ColorPickerButton(QColor color, QWidget *parent = nullptr);

    ~ColorPickerButton() override;

    void setTitle(QString title);
    void setColor(QColor color);
    QColor color() { return color_; }

Q_SIGNALS:
  void colorChanged(QColor new_color);

private:
    Ui::ColorPickerButton *ui;

    void registerEvents();
    void updateIcon(QColor color);

    QColor color_;
    QString title_;
};
