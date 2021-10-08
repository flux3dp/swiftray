#pragma once

#include <QDialog>


QT_BEGIN_NAMESPACE
namespace Ui { class ImageCropDialog; }
QT_END_NAMESPACE

class ImageCropDialog : public QDialog {
Q_OBJECT

public:
    explicit ImageCropDialog(QWidget *parent = nullptr);

    ~ImageCropDialog() override;

private:
    Ui::ImageCropDialog *ui;
};
