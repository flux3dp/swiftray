#ifndef JOBDASHBOARDDIALOG_H
#define JOBDASHBOARDDIALOG_H

#include <QDialog>
#include <widgets/base-container.h>
#include <motion_controller_job/base-job.h>

#include <QString>
#include <QPixmap>
#include <QTime>

namespace Ui {
class JobDashboardDialog;
}

class JobDashboardDialog : public QDialog, BaseContainer
{
    Q_OBJECT

public:
    explicit JobDashboardDialog(const QPixmap &preview, QWidget *parent = nullptr);
    explicit JobDashboardDialog(QTime total_required_time, const QPixmap &preview, QWidget *parent = nullptr);
    ~JobDashboardDialog();

    void attachJob(BaseJob *job);

public slots:
    void onStatusChanged(BaseJob::Status);
    void onProgressChanged(QVariant);
    void onElapsedTimeChanged(QTime);

signals:
    void startBtnClicked();
    void stopBtnClicked();
    void pauseBtnClicked();
    void resumeBtnClicked();
    void jobStatusReport(BaseJob::Status status);

private:
    void registerEvents() override;
    void loadStyles() override;

    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    QPixmap preview_;
    BaseJob::Status status_ = BaseJob::Status::READY;
    BaseJob *job_;

    Ui::JobDashboardDialog *ui;
};

#endif // JOBDASHBOARDDIALOG_H
