#pragma once

#include <QDialog>
#include <QGraphicsView>
#include <QGestureEvent>

#include <toolpath_exporter/generators/preview-generator.h>
#include <widgets/base-container.h>
#include <common/timestamp.h>

namespace Ui {
  class PreviewWindow;
}


class PreviewWindow : public QDialog, BaseContainer {
Q_OBJECT

public:

  explicit PreviewWindow(QWidget *parent, QRectF work_area, double scale);

  ~PreviewWindow();

  PreviewGenerator *previewPath() const;

  void setPreviewPath(std::shared_ptr<PreviewGenerator> &preview_path);

  void setRequiredTime(const Timestamp &required_time);

private:

  void registerEvents() override;
  void showEvent(QShowEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

  Ui::PreviewWindow *ui;
  std::shared_ptr<PreviewGenerator> preview_path_;
  QGraphicsView* path_graphics_view_;
  double height_scale_;
  double total_height_;
  QRectF work_area_;
};
