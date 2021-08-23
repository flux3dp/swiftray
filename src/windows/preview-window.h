#pragma once

#include <QDialog>
#include <gcode/generators/preview-generator.h>
#include <widgets/base-container.h>

#include <QGraphicsView>
#include <QGestureEvent>


namespace Ui {
  class PreviewWindow;
}


class PreviewWindow : public QDialog, BaseContainer {
Q_OBJECT

public:

  explicit PreviewWindow(QWidget *parent = nullptr);

  ~PreviewWindow();

  PreviewGenerator *previewPath() const;

  void setPreviewPath(std::shared_ptr<PreviewGenerator> &preview_path);

  void setRequiredTime(const QString &required_time);

private:

  void registerEvents() override;

  Ui::PreviewWindow *ui;
  int progress_;
  std::shared_ptr<PreviewGenerator> preview_path_;
  QGraphicsView* path_graphics_view_;
};
