#include "image-crop-dialog.h"
#include "ui_image-crop-dialog.h"


ImageCropDialog::ImageCropDialog(QWidget *parent) :
        QDialog(parent), ui(new Ui::ImageCropDialog) {
  ui->setupUi(this);
}

ImageCropDialog::~ImageCropDialog() {
  delete ui;
}

