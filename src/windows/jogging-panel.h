#ifndef JOGGINGPANEL_H
#define JOGGINGPANEL_H

#include <QDialog>
#include <settings/maintenance-controller.h>
#include <widgets/base-container.h>

namespace Ui {
class JoggingPanel;
}

class JoggingPanel : public QDialog
{
    Q_OBJECT

public:
    explicit JoggingPanel(QWidget *parent = nullptr);
    ~JoggingPanel();
    void close();

private:
    Ui::JoggingPanel *ui;
};

#endif // JOGGINGPANEL_H
