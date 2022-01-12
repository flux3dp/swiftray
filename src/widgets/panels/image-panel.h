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
    explicit ImagePanel(QWidget *parent, MainWindow *main_window);
    ~ImagePanel();

private:
    void registerEvents() override;

    Ui::ImagePanel *ui;
    MainWindow *main_window_;

};

#endif // IMAGEPANEL_H
