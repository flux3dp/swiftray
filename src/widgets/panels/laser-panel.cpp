#include "laser-panel.h"
#include "ui_laser-panel.h"
#include <windows/osxwindow.h>
#include <QDebug>

LaserPanel::LaserPanel(QWidget *parent, MainWindow *main_window) :
    QFrame(parent),
    ui(new Ui::LaserPanel),
    main_window_(main_window)
{
    ui->setupUi(this);
    setJobOrigin(job_origin_);
    initializeContainer();
    qRegisterMetaType<StartFrom>();
    ui->startFromComboBox->addItem(tr("Absolute Coords"), StartFrom::AbsoluteCoords);
    if(start_with_home_) {
        ui->homeCheckBox->setCheckState(Qt::Checked);
    }
    else {
        ui->homeCheckBox->setCheckState(Qt::Unchecked);
    }
    ui->startFromComboBox->addItem(tr("User Origin"), StartFrom::UserOrigin);
    ui->startFromComboBox->addItem(tr("Current Position"), StartFrom::CurrentPosition);
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
    connect(ui->moveToOriginBtn, &QAbstractButton::clicked, [=]() {
        Q_EMIT actionMoveToOrigin();
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
    connect(ui->startFromComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
        StartFrom start_from = ui->startFromComboBox->itemData(index).value<StartFrom>();
        start_from_ = start_from;
        if(start_from_ == AbsoluteCoords) {
            ui->widget->hide();
            ui->widget_2->show();
            start_with_home_ = ui->homeCheckBox->checkState() && ui->homeCheckBox->isEnabled();
        } else {
            ui->widget->show();
            ui->widget_2->hide();
            start_with_home_ = false;
        }
        Q_EMIT switchStartFrom(start_from_);
        Q_EMIT startWithHome(start_with_home_);
    });
    connect(ui->homeCheckBox, &QAbstractButton::clicked, [=](bool checked) {
        start_with_home_ = checked;
        Q_EMIT startWithHome(start_with_home_);
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

void LaserPanel::setStartFrom(StartFrom start_from)
{
    start_from_ = start_from;
}

int LaserPanel::getJobOrigin()
{
    return job_origin_;
}

int LaserPanel::getStartFrom()
{
    return start_from_;
}

bool LaserPanel::getStartWithHome()
{
    return start_with_home_;
}

void LaserPanel::setControlEnable(bool control_enable)
{
    ui->frameBtn->setEnabled(control_enable);
    ui->homeBtn->setEnabled(control_enable);
    ui->moveToOriginBtn->setEnabled(control_enable);
}

void LaserPanel::setStartHomeEnable(bool control_enable)
{
    if(!control_enable)
    {
        start_with_home_ = false;
    }
    else
    {
        start_with_home_ = ui->homeCheckBox->checkState();
    }
    ui->homeCheckBox->setEnabled(control_enable);
    Q_EMIT startWithHome(start_with_home_);
}

void LaserPanel::setLayout()
{
    ui->previewBtn->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-preview.png" : ":/resources/images/icon-preview.png"));
    ui->frameBtn->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-frame.png" : ":/resources/images/icon-frame.png"));
    ui->startBtn->setIcon(QIcon(isDarkMode() ? ":/resources/images/dark/icon-start.png" : ":/resources/images/icon-start.png"));
    ui->startFromComboBox->setCurrentIndex(0);
}

void LaserPanel::hideEvent(QHideEvent *event) {
  Q_EMIT panelShow(false);
}

void LaserPanel::showEvent(QShowEvent *event) {
  Q_EMIT panelShow(true);
}
