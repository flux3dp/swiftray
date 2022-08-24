#ifndef PRIVACY_WINDOW_H
#define PRIVACY_WINDOW_H

#include <QDialog>

namespace Ui {
class PrivacyWindow;
}

class PrivacyWindow : public QDialog
{
    Q_OBJECT

public:
    explicit PrivacyWindow(QWidget *parent = nullptr);
    ~PrivacyWindow();

private:
    Ui::PrivacyWindow *ui;

Q_SIGNALS:
    void privacyUpdate(bool enable_upload);
};

#endif // PRIVACY_WINDOW_H
