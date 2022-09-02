#include "privacy_window.h"
#include "ui_privacy_window.h"

PrivacyWindow::PrivacyWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PrivacyWindow)
{
    ui->setupUi(this);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, [=](){
        Q_EMIT privacyUpdate(true);
    });
    connect(ui->buttonBox, &QDialogButtonBox::rejected, [=](){
        Q_EMIT privacyUpdate(false);
    });
}

PrivacyWindow::~PrivacyWindow()
{
    delete ui;
}
