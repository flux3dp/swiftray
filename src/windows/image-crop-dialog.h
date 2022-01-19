#pragma once

#include <QDialog>
#include <widgets/base-container.h>


QT_BEGIN_NAMESPACE
namespace Ui { class ImageCropDialog; }
QT_END_NAMESPACE

class ImageCropDialog : public QDialog, BaseContainer {
Q_OBJECT

public:
    explicit ImageCropDialog(QWidget *parent = nullptr);

    ~ImageCropDialog() override;

    void loadImage(const QImage &img);
    void initBackgroundDisplay();
    QPixmap getCrop();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void registerEvents() override;
    void loadStyles() override;

    Ui::ImageCropDialog *ui;

    QImage src_image_;
};
