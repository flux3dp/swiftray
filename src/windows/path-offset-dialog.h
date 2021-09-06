#pragma once

#include <QDialog>
#include <widgets//base-container.h>
#include <clipper/clipper.hpp>

QT_BEGIN_NAMESPACE
namespace Ui { class PathOffsetDialog; }
QT_END_NAMESPACE

class PathOffsetDialog : public QDialog, BaseContainer {
Q_OBJECT

public:
    explicit PathOffsetDialog(QWidget *parent = nullptr);

    ~PathOffsetDialog() override;

    void reset();
    void addPath(QPolygonF path);
    void updatePathOffset();
    QList<QPolygonF>& getResult() { return offset_path_list_; };

private:
    Ui::PathOffsetDialog *ui;
    void registerEvents() override;
    void loadStyles() override;

    ClipperLib::Paths convert_for_clipper(bool closed);
    void convert_from_clipper(ClipperLib::Paths clipper_result);

    static constexpr qreal scale_factor = 100;
    static constexpr qreal scale_factor_invert = 1/scale_factor;
    QTransform scale_up_ = QTransform::fromScale(scale_factor, scale_factor);
    QTransform scale_down_ = QTransform::fromScale(scale_factor_invert, scale_factor_invert);
    QList<QPolygonF> path_list_; // source path
    QList<QPolygonF> offset_path_list_; // result
};

