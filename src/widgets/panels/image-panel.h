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

    void setLayout();

signals:
    void panelShow(bool is_show);

private:
    void loadStyles() override;

    void registerEvents() override;

    void hideEvent(QHideEvent *event) override;

    void showEvent(QShowEvent *event) override;

    Ui::ImagePanel *ui;
    MainWindow *main_window_;

};

#endif // IMAGEPANEL_H
