#include "image-panel.h"
#include "ui_image-panel.h"

#include <canvas/canvas.h>
#include <shape/shape.h>
#include <shape/bitmap-shape.h>
#include <windows/mainwindow.h>

ImagePanel::ImagePanel(QWidget *parent, MainWindow *main_window) :
    QFrame(parent),
    ui(new Ui::ImagePanel),
    main_window_(main_window)
{
    ui->setupUi(this);
    ui->checkBox->setCheckState(Qt::Checked);
    ui->checkBox->setEnabled(false);
    ui->horizontalSlider->setValue(128);
    ui->horizontalSlider->setEnabled(false);
    initializeContainer();
}

ImagePanel::~ImagePanel()
{
    delete ui;
}

void ImagePanel::registerEvents() {
  connect(main_window_->canvas(), &Canvas::selectionsChanged, this, [=]() {
    if (main_window_->canvas()->document().selections().size() == 1 &&
        main_window_->canvas()->document().selections().at(0)->type() == ::Shape::Type::Bitmap) {
      BitmapShape* selected_img = dynamic_cast<BitmapShape *>(main_window_->canvas()->document().selections().at(0).get());
      ui->checkBox->setCheckState(selected_img->gradient() ? Qt::Checked : Qt::Unchecked);
      ui->horizontalSlider->setValue(selected_img->thrsh_brightness());
      ui->checkBox->setEnabled(true);
      ui->horizontalSlider->setEnabled(true);

      if (selected_img->gradient()) {
        ui->horizontalSlider->setVisible(false);
        ui->label->setVisible(false);
      } else {
        ui->horizontalSlider->setVisible(true);
        ui->label->setVisible(true);
      }

      setEnabled(true);
    } else {
      ui->checkBox->setEnabled(false);
      ui->horizontalSlider->setEnabled(false);
      setEnabled(false);
    }
  });

  connect(ui->checkBox, QOverload<int>::of(&QCheckBox::stateChanged), [=](int state) {
    if (main_window_->canvas()->document().selections().size() == 1 &&
        main_window_->canvas()->document().selections().at(0)->type() == ::Shape::Type::Bitmap) {
      BitmapShape* selected_img = dynamic_cast<BitmapShape *>(main_window_->canvas()->document().selections().at(0).get());
      selected_img->setGradient(state);
      if (state) {
        ui->horizontalSlider->setVisible(false);
        ui->label->setVisible(false);
      } else {
        ui->horizontalSlider->setVisible(true);
        ui->label->setVisible(true);
      }
    }
  });

  connect(ui->horizontalSlider, QOverload<int>::of(&QSlider::valueChanged), [=](int value) {
      if (main_window_->canvas()->document().selections().size() == 1 &&
          main_window_->canvas()->document().selections().at(0)->type() == ::Shape::Type::Bitmap) {
        BitmapShape* selected_img = dynamic_cast<BitmapShape *>(main_window_->canvas()->document().selections().at(0).get());
        selected_img->setThrshBrightness(value);
      }
  });
}

