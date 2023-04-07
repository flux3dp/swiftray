#ifndef IMAGEPANEL_H
#define IMAGEPANEL_H

#include <QFrame>
#include <widgets/base-container.h>

class MainWindow;

namespace Ui {
class ImagePanel;
}

class ImagePanel : public QFrame, BaseContainer
{
    Q_OBJECT

public:
    explicit ImagePanel(QWidget *parent, bool is_dark_mode);
    ~ImagePanel();
    void setLayout(bool is_dark_mode);
    void setImageGradient(bool state);
    void setImageThreshold(int value);
    void changeImageEnable(bool enable);

Q_SIGNALS:
    void editImageGradient(bool state);
    void editImageThreshold(int value);
    void actionCropImage();
    void actionInvertImage();
    void actionSharpenImage();
    void actionGenImageTrace();
    void panelShow(bool is_show);

private:
    void loadStyles() override;
    void registerEvents() override;
    void hideEvent(QHideEvent *event) override;
    void showEvent(QShowEvent *event) override;

    Ui::ImagePanel *ui;
};

#endif // IMAGEPANEL_H
