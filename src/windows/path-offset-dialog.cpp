#include "path-offset-dialog.h"
#include "ui_path-offset-dialog.h"

#include <QPushButton>
#include <QPainterPath>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QDebug>
#include <clipper/clipper.hpp>
#include <list>

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

  QPen pen{Qt::black};
  pen.setWidth(3);
  pen.setCosmetic(true);
  if (path.isClosed()) {
    QGraphicsPolygonItem *closed_path_item = new QGraphicsPolygonItem(path);
    closed_path_item->setPen(pen);
    ui->graphicsView->scene()->addItem(closed_path_item);
  } else {
    // NOTE: open polygon can't be added directly to graphicsscene
    QPainterPath open_path;
    open_path.addPolygon(path);
    QGraphicsPathItem *open_path_item = new QGraphicsPathItem(open_path);
    open_path_item->setPen(pen);
    ui->graphicsView->scene()->addItem(open_path_item);
  }
}

ClipperLib::Paths PathOffsetDialog::convertQtToClipper(bool closed_path) {
  ClipperLib::Paths clipper_paths;
  for (auto polygon_path: path_list_) {
    if (closed_path && !polygon_path.isClosed()) {
      continue;
    }
    if (!closed_path && polygon_path.isClosed()) {
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

void PathOffsetDialog::convertClipperToQt(ClipperLib::Paths clipper_paths) {
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
    if (item->data(ITEM_ID_KEY).toString().compare(QString(PATH_OFFSET_ITEM_ID)) == 0) {
      ui->graphicsView->scene()->removeItem(item);
    }
  }

  ClipperLib::ClipperOffset clipper_offset;
  // Separate closed and open source polygons
  ClipperLib::Paths input_clipper_paths = convertQtToClipper(false);
  ClipperLib::Paths input_closed_clipper_paths = convertQtToClipper(true);

  if (ui->cornerComboBox->currentIndex() == 1) {
    clipper_offset.AddPaths(input_clipper_paths, ClipperLib::jtRound, ClipperLib::etOpenRound);
    clipper_offset.AddPaths(input_closed_clipper_paths, ClipperLib::jtRound, ClipperLib::etOpenRound);
  } else {
    clipper_offset.AddPaths(input_clipper_paths, ClipperLib::jtMiter, ClipperLib::etOpenSquare);
    clipper_offset.AddPaths(input_closed_clipper_paths, ClipperLib::jtMiter, ClipperLib::etClosedLine);
  }
  ClipperLib::Paths output_clipper_paths;
  clipper_offset.Execute(output_clipper_paths, ui->distanceDoubleSpinBox->value() * 10 * scale_factor_);
  convertClipperToQt(output_clipper_paths);

  if (offset_path_list_.count() == 0) {
    return;
  }

  // Only keep inward/outward offset depending on user selection
  if (ui->directionComboBox->currentIndex() == 0) {
    offset_path_list_ = extractUnidirectionalOffsetPaths(offset_path_list_, false);
  } else {
    offset_path_list_ = extractUnidirectionalOffsetPaths(offset_path_list_, true);
  }

  // Draw new result
  for (auto offset_path: offset_path_list_) {
    QPen pen{Qt::green};
    pen.setWidth(3);
    pen.setCosmetic(true);
    QGraphicsPolygonItem *offset_item = new QGraphicsPolygonItem(offset_path);
    offset_item->setPen(pen);
    offset_item->setData(ITEM_ID_KEY, PATH_OFFSET_ITEM_ID);
    ui->graphicsView->scene()->addItem(offset_item);
  }
}

/**
 *
 * @param origin_list
 * @param extract_inward true for extracting only inward offset paths
 *                       false for extracting only outward offset paths
 * @return
 */
QList<QPolygonF> PathOffsetDialog::extractUnidirectionalOffsetPaths(
        const QList<QPolygonF>& origin_list, bool extract_inward) {
  QList<QPolygonF> inward_list, outward_list;
  std::list<int> index_list; // NOTE: QList has different behavior from std::list in erase operation
  for (auto i = 0; i < offset_path_list_.count(); i++) {
    index_list.push_back(i);
  }

  auto it1 = index_list.begin();
  while(it1 != index_list.end()) {
    auto i = *it1;
    auto it2 = it1;
    it2++;
    while(it2 != index_list.end()) {
      auto j = *it2;
      if (origin_list.at(i).containsPoint(
              origin_list.at(j).first(), Qt::OddEvenFill)) {
        inward_list.push_back(offset_path_list_.at(j));
        it2 = index_list.erase(it2);
        continue;
      } else if (origin_list.at(j).containsPoint(
              origin_list.at(i).first(), Qt::OddEvenFill)) {
        break; // early terminate -> i is inward
      } else {
        it2++;
      }
    }
    if (it2 == index_list.end()) {
      outward_list.push_back(origin_list.at(i));
    } else {
      inward_list.push_back(origin_list.at(i));
    }
    it1 = index_list.erase(it1);
  }

  return extract_inward ? inward_list : outward_list;
}
