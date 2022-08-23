#pragma once

#include <QDialog>
#include <widgets//base-container.h>
#include <memory>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgproc/types_c.h>


QT_BEGIN_NAMESPACE
namespace Ui { class ImageSharpenDialog; }
QT_END_NAMESPACE

class ImageSharpenDialog : public QDialog, BaseContainer {
    Q_OBJECT

public:
    explicit ImageSharpenDialog(QWidget *parent = nullptr);

    ~ImageSharpenDialog() override;

    void reset();
    void resetParams();
    void loadImage(const QImage *img);
    void loadImage(const QImage &img);
    void updateBackgroundDisplay();
    void updateImageSharpen();
    QImage getSharpenedImage() { return sharpened_image_; };

public Q_SLOTS:
    void onSharpnessChanged(int new_sharpness_val);
    void onRadiusChanged(int new_radius_val);

protected:
    void showEvent(QShowEvent *event) override;

private:

    Ui::ImageSharpenDialog *ui;
    void registerEvents() override;
    void loadStyles() override;

    QImage src_image_grayscale_;
    QImage sharpened_image_;
    QImage ImageBinarize(const QImage &image, int threshold, int cutoff);
    QImage FadeImage(const QImage &image);
    //QPainterPath contours_;

    // used to prevent duplicate Q_SIGNALS
    void setSharpnessSpinboxWithoutEmit(int sharpness);
    void setSharpnessSliderWithoutEmit(int sharpness);
    void setRadiusSpinboxWithoutEmit(int radius);
    void setRadiusSliderWithoutEmit(int radius);
};