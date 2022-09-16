#include "laser-panel.h"
#include "ui_laser-panel.h"
#include <windows/osxwindow.h>

LaserPanel::LaserPanel(QWidget *parent, MainWindow *main_window) :
    QFrame(parent),
    ui(new Ui::LaserPanel),
    main_window_(main_window)
{
    ui->setupUi(this);
    ui->toOriginBtn->hide();
    setJobOrigin(job_origin_);
    initializeContainer();
    setLayout();
}

void LaserPanel::loadStyles() {
}

void LaserPanel::registerEvents() {
    connect(ui->previewBtn, &QAbstractButton::clicked, [=]() {
        Q_EMIT actionPreview();
    });
    connect(ui->frameBtn, &QAbstractButton::clicked, [=]() {
        Q_EMIT actionFrame();
    });
    connect(ui->startBtn, &QAbstractButton::clicked, [=]() {
        Q_EMIT actionStart();
    });
    connect(ui->homeBtn, &QAbstractButton::clicked, [=]() {
        Q_EMIT actionHome();
    });
    connect(ui->NWRadioButton, &QAbstractButton::clicked, [=](bool checked) {
        job_origin_ = NW;
        Q_EMIT selectJobOrigin(NW);
    });
    connect(ui->NRadioButton, &QAbstractButton::clicked, [=](bool checked) {
        job_origin_ = N;
        Q_EMIT selectJobOrigin(N);
    });
    connect(ui->NERadioButton, &QAbstractButton::clicked, [=](bool checked) {
        job_origin_ = NE;
        Q_EMIT selectJobOrigin(NE);
    });
    connect(ui->WRadioButton, &QAbstractButton::clicked, [=](bool checked) {
        job_origin_ = W;
        Q_EMIT selectJobOrigin(W);
    });
    connect(ui->CRadioButton, &QAbstractButton::clicked, [=](bool checked) {
        job_origin_ = CENTER;
        Q_EMIT selectJobOrigin(CENTER);
    });
    connect(ui->ERadioButton, &QAbstractButton::clicked, [=](bool checked) {
        job_origin_ = E;
        Q_EMIT selectJobOrigin(E);
    });
    connect(ui->SWRadioButton, &QAbstractButton::clicked, [=](bool checked) {
        job_origin_ = SW;
        Q_EMIT selectJobOrigin(SW);
    });
    connect(ui->SRadioButton, &QAbstractButton::clicked, [=](bool checked) {
        job_origin_ = S;
        Q_EMIT selectJobOrigin(S);
    });
    connect(ui->SERadioButton, &QAbstractButton::clicked, [=](bool checked) {
        job_origin_ = SE;
        Q_EMIT selectJobOrigin(SE);
    });
    connect(ui->comboBox, &QComboBox::currentTextChanged, [=](const QString &text) {
        if(text == "Current Position") {
            ui->widget->show();
        } else {
            ui->widget->hide();
        }
    });
}

LaserPanel::~LaserPanel()
{
    delete ui;
}

void LaserPanel::setJobOrigin(JobOrigin position)
{
    job_origin_ = position;
    switch(job_origin_) {
        case NW:
            ui->NWRadioButton->setChecked(true);
            break;
        case N:
            ui->NRadioButton->setChecked(true);
            break;
        case NE:
            ui->NERadioButton->setChecked(true);
            break;
        case E:
            ui->ERadioButton->setChecked(true);
            break;
        case SE:
            ui->SERadioButton->setChecked(true);
            break;
        case S:
            ui->SRadioButton->setChecked(true);
            break;
        case SW:
            ui->SWRadioButton->setChecked(true);
            break;
        case W:
            ui->WRadioButton->setChecked(true);
            break;
        case CENTER:
            ui->CRadioButton->setChecked(true);
            break;
    }
}

int LaserPanel::getJobOrigin()
{
    return job_origin_;
}

void LaserPanel::setLayout()
{
    ui->previewBtn->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-preview.png" : ":/resources/images/icon-preview.png"));
    ui->frameBtn->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-frame.png" : ":/resources/images/icon-frame.png"));
    ui->startBtn->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-start.png" : ":/resources/images/icon-start.png"));
    ui->comboBox->setCurrentIndex(0);
    ui->widget->hide();
}

void LaserPanel::hideEvent(QHideEvent *event) {
  emit panelShow(false);
}

void LaserPanel::showEvent(QShowEvent *event) {
  emit panelShow(true);
}