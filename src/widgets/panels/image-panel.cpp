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
    ui->gradientCheckBox->setCheckState(Qt::Checked);
    ui->brightnessThresholdSlider->setValue(128);
    setEnabled(false);
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
      setEnabled(true);
      // NOTE:
      //     For unknown reason, the group widget and slider inside it must be enabled manually
      //     From Qt documentation, all child widgets should be enabled when the parent is enabled
      //     (if not individually specified)
      ui->brightnessThresholdGroup->setEnabled(true);
      ui->brightnessThresholdSlider->setEnabled(true);

      ui->gradientCheckBox->setCheckState(selected_img->gradient() ? Qt::Checked : Qt::Unchecked);
      ui->brightnessThresholdSlider->setValue(selected_img->thrsh_brightness());
      if (selected_img->gradient()) {
        ui->brightnessThresholdGroup->setVisible(false);
      } else {
        ui->brightnessThresholdGroup->setVisible(true);
      }
    } else {
      // Disable the parent widget will disable all child widgets as well
      setEnabled(false);
    }
  });

  connect(ui->gradientCheckBox, QOverload<int>::of(&QCheckBox::stateChanged), [=](int state) {
    if (main_window_->canvas()->document().selections().size() == 1 &&
        main_window_->canvas()->document().selections().at(0)->type() == ::Shape::Type::Bitmap) {
      BitmapShape* selected_img = dynamic_cast<BitmapShape *>(main_window_->canvas()->document().selections().at(0).get());
      selected_img->setGradient(state);
      if (state) {
        ui->brightnessThresholdGroup->setVisible(false);
      } else {
        ui->brightnessThresholdGroup->setVisible(true);
      }
    }
  });

  connect(ui->brightnessThresholdSlider, QOverload<int>::of(&QSlider::valueChanged), [=](int value) {
      if (main_window_->canvas()->document().selections().size() == 1 &&
          main_window_->canvas()->document().selections().at(0)->type() == ::Shape::Type::Bitmap) {
        BitmapShape* selected_img = dynamic_cast<BitmapShape *>(main_window_->canvas()->document().selections().at(0).get());
        selected_img->setThrshBrightness(value);
      }
  });
}

