#pragma once

#include <QDialog>
#include <toolpath_exporter/generators/preview-generator.h>
#include <widgets/base-container.h>

#include <QGraphicsView>
#include <QGestureEvent>
#include <QTime>


namespace Ui {
  class PreviewWindow;
}


class PreviewWindow : public QDialog, BaseContainer {
Q_OBJECT

public:

  explicit PreviewWindow(QWidget *parent, int width, int height);

  ~PreviewWindow();

  PreviewGenerator *previewPath() const;

  void setPreviewPath(std::shared_ptr<PreviewGenerator> &preview_path);

  void setRequiredTime(const QTime &required_time);

private:

  void registerEvents() override;
  void showEvent(QShowEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;

  Ui::PreviewWindow *ui;
  std::shared_ptr<PreviewGenerator> preview_path_;
  QGraphicsView* path_graphics_view_;
};
