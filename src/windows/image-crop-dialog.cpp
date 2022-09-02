#include "image-crop-dialog.h"
#include "ui_image-crop-dialog.h"

#include <QDebug>
#include <QPushButton>


ImageCropDialog::ImageCropDialog(QWidget *parent) :
        QDialog(parent), ui(new Ui::ImageCropDialog) {
  ui->setupUi(this);
  loadStyles();
  registerEvents();
}

ImageCropDialog::~ImageCropDialog() {
  delete ui;
}

void ImageCropDialog::registerEvents() {
  connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked,
          this, &ImageCropDialog::accept);
  connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
          this, &ImageCropDialog::reject);
  connect(ui->buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
          this, [=] () {
    ui->graphicsView->updateTargetImage(ui->graphicsView->getCropImage());
  });
  connect(ui->buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked,
          this, [=] () {
    this->resetCropOperation(); // re-initialize target_image with src_image
  });
}

void ImageCropDialog::loadStyles() {
  ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Ok"));
  ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
  ui->buttonBox->button(QDialogButtonBox::Apply)->setText(tr("Apply"));
  ui->buttonBox->button(QDialogButtonBox::Reset)->setText(tr("Reset"));
}

void ImageCropDialog::showEvent(QShowEvent *event) {
  resetCropOperation();
}

void ImageCropDialog::loadImage(const QImage &img) {
  src_image_ = img;
  try {
    ui->graphicsView->scene()->setSceneRect(0, 0,
                                                 src_image_.width(),
                                                 src_image_.height());
  } catch (const std::exception& e) {
    qInfo() << e.what();
    return;
  }
}

/**
 * @brief Create background pixmap based on background mode
 *        and update it on the graphicsscene
 */
void ImageCropDialog::resetCropOperation() {
  if (src_image_.isNull()) {
    return;
  }
  ui->graphicsView->updateTargetImage(src_image_);
  ui->graphicsView->fitInView(ui->graphicsView->sceneRect(), Qt::KeepAspectRatio);
}

QImage ImageCropDialog::getCropImage() {
  return ui->graphicsView->getCropImage();
}
