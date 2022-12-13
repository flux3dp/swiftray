#pragma once

#include <QDialog>
#include <widgets//base-container.h>
#include <memory>
#include <QxPotrace/include/qxpotrace.h>


QT_BEGIN_NAMESPACE
namespace Ui { class ImageTraceDialog; }
QT_END_NAMESPACE

class ImageTraceDialog : public QDialog, BaseContainer {
    Q_OBJECT

public:
    explicit ImageTraceDialog(QWidget *parent = nullptr);

    ~ImageTraceDialog() override;

    void reset();
    void resetParams();
    void loadImage(const QImage &img);
    void updateBackgroundDisplay();
    void updateImageTrace();

    QPainterPath getTrace() { return contours_; }
    bool shouldDeleteImg();

public Q_SLOTS:
    void onSelectPartialStateChanged(int state);
    void onCutoffChanged(int new_cutoff_val);
    void onThresholdChanged(int new_thres_val);

private:

    Ui::ImageTraceDialog *ui;
    void registerEvents() override;
    void loadStyles() override;
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    QImage src_image_grayscale_;
    QImage ImageBinarize(const QImage &image, int threshold, int cutoff);
    QImage FadeImage(const QImage &image);
    QImage createSubImage(QImage* image, const QRect & rect);
    std::shared_ptr<QxPotrace> potrace_;
    QPainterPath contours_;

    // used to prevent duplicate Q_SIGNALS
    void setCutoffSpinboxWithoutEmit(int cutoff);
    void setCutoffSliderWithoutEmit(int cutoff);
    void setThresholdSpinboxWithoutEmit(int thres);
    void setThresholdSliderWithoutEmit(int thres);
};