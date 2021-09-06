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
    // NOTE: open polygon can't be added directly to graphicsscene
    QPainterPath open_path;
    open_path.addPolygon(path);
    QGraphicsPathItem *contour = new QGraphicsPathItem(open_path);
    QGraphicsRectItem rect;
    ui->graphicsView->scene()->addItem(contour);
  }

}

ClipperLib::Paths PathOffsetDialog::convert_for_clipper(bool closed) {
  ClipperLib::Paths clipper_paths;
  for (auto polygon_path: path_list_) {
    if (closed && !polygon_path.isClosed()) {
      continue;
    }
    if (!closed && polygon_path.isClosed()) {
      continue;
    }
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
  // Separate closed and open source polygons
  ClipperLib::Paths input_clipper_paths = convert_for_clipper(false);
  ClipperLib::Paths input_closed_clipper_paths = convert_for_clipper(true);

  if (ui->cornerComboBox->currentIndex() == 1) {
    clipper_offset.AddPaths(input_clipper_paths, ClipperLib::jtRound, ClipperLib::etOpenRound);
    clipper_offset.AddPaths(input_closed_clipper_paths, ClipperLib::jtRound, ClipperLib::etOpenRound);
  } else {
    clipper_offset.AddPaths(input_clipper_paths, ClipperLib::jtMiter, ClipperLib::etOpenSquare);
    clipper_offset.AddPaths(input_closed_clipper_paths, ClipperLib::jtMiter, ClipperLib::etClosedLine);
  }
  ClipperLib::Paths output_clipper_paths;
  clipper_offset.Execute(output_clipper_paths, ui->distanceDoubleSpinBox->value() * scale_factor);
  convert_from_clipper(output_clipper_paths);

  if (offset_path_list_.count() == 0) {
    return;
  }

  // Separate outward offset and inward offset
  QList<QPolygonF> inward;
  QList<QPolygonF> outward;
  QList<int> processed_idx_list;
  for (auto i = 0; i < offset_path_list_.count(); i++) {
    if (processed_idx_list.contains(i)) { // filter out processed item
      continue;
    }
    bool is_contained_by = false; // whether i is inside any other path
    for (auto j = i + 1; j < offset_path_list_.count(); j++) {
      if (processed_idx_list.contains(j)) { // filter out processed item
        continue;
      }
      if (offset_path_list_.at(i).containsPoint(offset_path_list_.at(j).first(), Qt::OddEvenFill)) {
        // i contains j -> j is an inward
        inward.push_back(offset_path_list_.at(j));
        processed_idx_list.push_back(j);
      }
      else if (offset_path_list_.at(j).containsPoint(offset_path_list_.at(i).first(), Qt::OddEvenFill)) {
        // j contains i -> i is an inward
        is_contained_by = true;
        break; // exit current i-th loop
      }
    }
    if (is_contained_by) {
      inward.push_back(offset_path_list_.at(i));
    } else {
      outward.push_back(offset_path_list_.at(i));
    }
  }
  // Only keep inward/outward offset depending on user selection
  if (ui->directionComboBox->currentIndex() == 0) {
    offset_path_list_ = outward;
  } else {
    offset_path_list_ = inward;
  }

  // draw new result
  for (auto offset_path: offset_path_list_) {
    auto poly_item = ui->graphicsView->scene()->addPolygon(offset_path, QPen{QColor{Qt::blue}});
    poly_item->setData(0, "PATH_OFFSET");
  }
}
