#pragma once

#include <QDialog>
#include <widgets//base-container.h>
#include <opencv2/opencv.hpp>
#include <memory>
#include <QxPotrace/include/qxpotrace.h>
#include <QGraphicsPathItem>


QT_BEGIN_NAMESPACE
namespace Ui { class ImageTraceDialog; }
QT_END_NAMESPACE

class ImageTraceDialog : public QDialog, BaseContainer {
    Q_OBJECT

public:
    explicit ImageTraceDialog(QWidget *parent = nullptr);

    ~ImageTraceDialog() override;

    void resetParams();
    void loadImage(const QImage *img);
    void onCutoffChanged(int new_cutoff_val);
    void onThresholdChanged(int new_thres_val);
    void updateBackgroundDisplay();
    void updateImageTrace();

public slots:
    void onSelectPartialStateChanged(int state);

private:

    Ui::ImageTraceDialog *ui;
    void registerEvents() override;
    QImage ImageToGrayscale(const QImage &image);
};