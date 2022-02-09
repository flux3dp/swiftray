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
    PathOffsetDialog(QSizeF document_size, QWidget *parent = nullptr);
    PathOffsetDialog(QWidget *parent = nullptr) = delete;

    ~PathOffsetDialog() override;

    void reset();
    void addPath(QPolygonF path);
    void updatePathOffset();
    QList<QPolygonF>& getResult() { return offset_path_list_; };

private:
    Ui::PathOffsetDialog *ui;
    void registerEvents() override;
    void loadStyles() override;
    void showEvent(QShowEvent *event) override;

    ClipperLib::Paths convertQtToClipper(bool closed_path);
    void convertClipperToQt(ClipperLib::Paths clipper_result);
    QList<QPolygonF> extractUnidirectionalOffsetPaths(const QList<QPolygonF>& origin_list, bool extract_inward);

    qreal scale_factor_ = 100;
    qreal scale_factor_invert_ = 1/scale_factor_;
    QTransform scale_up_ = QTransform::fromScale(scale_factor_, scale_factor_);
    QTransform scale_down_ = QTransform::fromScale(scale_factor_invert_, scale_factor_invert_);
    QList<QPolygonF> path_list_; // source path
    QList<QPolygonF> offset_path_list_; // result
    QSizeF document_size_ = QSizeF{3000, 2000}; // default value

    constexpr static int ITEM_ID_KEY = 0;
    constexpr static char PATH_OFFSET_ITEM_ID[] = "PATH_OFFSET";
};

