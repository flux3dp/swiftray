#include "path-offset-dialog.h"
#include "ui_path-offset-dialog.h"

#include <QPushButton>
#include <QPainterPath>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QDebug>
#include <clipper/clipper.hpp>

PathOffsetDialog::PathOffsetDialog(QWidget *parent) :
        QDialog(parent), ui(new Ui::PathOffsetDialog) {
  ui->setupUi(this);

  initializeContainer();
  reset();
  ui->graphicsView->setMinScale(0.4);
  ui->graphicsView->scale(0.3, 0.3);
}

PathOffsetDialog::~PathOffsetDialog() {
  delete ui;
}

void PathOffsetDialog::reset() {
  // reset GUI input
  ui->cornerComboBox->setCurrentIndex(0);
  ui->distanceDoubleSpinBox->setValue(1.0);
  ui->directionComboBox->setCurrentIndex(0);

  // reset internal data
  path_list_.clear();
  offset_path_list_.clear();

  // reset graphicsview
  QGraphicsScene *scene = new QGraphicsScene();
  scene->setSceneRect(QRectF{0, 0, 3000, 2000});
  scene->setBackgroundBrush(Qt::white);
  ui->graphicsView->setScene(scene);
}

void PathOffsetDialog::registerEvents() {
  connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked,
          this, &PathOffsetDialog::accept);
  connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
          this, &PathOffsetDialog::reject);

  connect(ui->distanceDoubleSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), [=]() {
    this->updatePathOffset();
  });
  connect(ui->cornerComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=]() {
    this->updatePathOffset();
  });
  connect(ui->directionComboBox, qOverload<int>(&QComboBox::currentIndexChanged), [=]() {
      this->updatePathOffset();
  });
}

void PathOffsetDialog::loadStyles() {
  ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
  ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
}

void PathOffsetDialog::addPath(QPolygonF path) {
  path_list_ << path;
  //ui->graphicsView->scene()->addPolygon(path);

  if (path.isClosed()) {
    ui->graphicsView->scene()->addPolygon(path);
  } else {
    QPainterPath open_path;
    open_path.addPolygon(path);
    QGraphicsPathItem *contour = new QGraphicsPathItem(open_path);
    QGraphicsRectItem rect;
    ui->graphicsView->scene()->addItem(contour);
  }

}

ClipperLib::Paths PathOffsetDialog::convert_for_clipper() {
  ClipperLib::Paths clipper_paths;
  for (auto polygon_path: path_list_) {
    ClipperLib::Path clipper_path;
    for (auto point: scale_up_.map(polygon_path).toPolygon()) {
      clipper_path.push_back(ClipperLib::IntPoint{point.x(), point.y()});
    }
    clipper_paths << clipper_path;
  }
  return clipper_paths;
}

void PathOffsetDialog::convert_from_clipper(ClipperLib::Paths clipper_paths) {
  for (auto clipper_path: clipper_paths) {
    QPolygonF poly_path;
    for (auto point: clipper_path) {
      poly_path << QPointF{qreal(point.X), qreal(point.Y)};
    }

    // close the polygon by connecting to the first point
    poly_path << QPointF{qreal(clipper_path.front().X), qreal(clipper_path.front().Y)};
    offset_path_list_ << scale_down_.map(poly_path);
  }
}

void PathOffsetDialog::updatePathOffset() {
  // clear first
  offset_path_list_.clear();
  for (auto item: ui->graphicsView->scene()->items()) {
    if (item->data(0).toString().compare(QString("PATH_OFFSET")) == 0) {
      ui->graphicsView->scene()->removeItem(item);
    }
  }

  ClipperLib::ClipperOffset clipper_offset;
  // TODO: Separate closed path & open path
  //       QPolygon.isClosed()
  ClipperLib::Paths input_clipper_paths = convert_for_clipper();

  if (ui->cornerComboBox->currentIndex() == 1) {
    clipper_offset.AddPaths(input_clipper_paths, ClipperLib::jtRound, ClipperLib::etOpenRound);
  } else {
    // TODO: apply different EndType and JoinType to closed path & open path
    //clipper_offset.AddPaths(input_clipper_paths, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
  }
  ClipperLib::Paths output_clipper_paths;
  clipper_offset.Execute(output_clipper_paths, ui->distanceDoubleSpinBox->value() * scale_factor);
  convert_from_clipper(output_clipper_paths);

  if (offset_path_list_.count() == 0) {
    return;
  }
  //find the outward offset
  int max_path_index = 0;
  QSizeF max_bound{0, 0};
  for (auto i = 0; i < offset_path_list_.count(); i++) {
    auto offset_path = offset_path_list_.at(i);
    if (max_bound.width() <= offset_path.boundingRect().width() &&
        max_bound.height() <= offset_path.boundingRect().height()) {
      max_bound.setWidth(offset_path.boundingRect().width());
      max_bound.setHeight(offset_path.boundingRect().height());
      max_path_index = i;
    }
  }
  if (ui->directionComboBox->currentIndex() == 1) {
    // only keep inward offset (delete outward offset)
    offset_path_list_.removeAt(max_path_index);
  } else {
    // only keep outward offset
    auto outward_path_offset = offset_path_list_.at(max_path_index);
    offset_path_list_.clear();
    offset_path_list_.push_back(outward_path_offset);
  }

  // draw new result
  for (auto offset_path: offset_path_list_) {
    auto poly_item = ui->graphicsView->scene()->addPolygon(offset_path, QPen{QColor{Qt::blue}});
    poly_item->setData(0, "PATH_OFFSET");
  }
}
