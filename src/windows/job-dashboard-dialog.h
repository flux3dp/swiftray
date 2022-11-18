#ifndef JOBDASHBOARDDIALOG_H
#define JOBDASHBOARDDIALOG_H

#include <QDialog>
#include <QPointer>
#include <QString>
#include <QPixmap>
#include <QVariant>
#include <widgets/base-container.h>
#include <executor/executor.h>
#include <executor/job_executor.h>
#include <common/timestamp.h>

namespace Ui {
class JobDashboardDialog;
}

class JobDashboardDialog : public QDialog, BaseContainer
{
    Q_OBJECT

public:
    explicit JobDashboardDialog(const QPixmap &preview, QWidget *parent = nullptr);
    explicit JobDashboardDialog(Timestamp total_required_time, const QPixmap &preview, QWidget *parent = nullptr);
    ~JobDashboardDialog();

    void attachJob(QPointer<JobExecutor> job_executor);
    QPixmap getPreview();

public Q_SLOTS:
    void onJobStateChanged(Executor::State);
    void onJobProgressChanged(QVariant);
    void onElapsedTimeChanged(Timestamp);

Q_SIGNALS:
    void startBtnClicked();
    void stopBtnClicked();
    void pauseBtnClicked();
    void resumeBtnClicked();
    void jobStatusReport(Executor::State);

private:
    void registerEvents() override;
    void loadStyles() override;

    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    QPixmap preview_;
    Executor::State job_state_ = Executor::State::kIdle;
    Timestamp total_required_time_;
    QPointer<JobExecutor> job_executor_;

    Ui::JobDashboardDialog *ui;
};

#endif // JOBDASHBOARDDIALOG_H
