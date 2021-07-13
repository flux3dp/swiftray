#ifndef PREVIEWWINDOW_H
#define PREVIEWWINDOW_H

#include <QDialog>
#include <gcode/generators/preview-generator.h>
#include <widgets/base-container.h>

namespace Ui {
  class PreviewWindow;
}

class PreviewWindow : public QDialog, BaseContainer {
Q_OBJECT

public:

  explicit PreviewWindow(QWidget *parent = nullptr);

  ~PreviewWindow();

  void paintEvent(QPaintEvent *event) override;

  PreviewGenerator *previewPath() const;

  void setPreviewPath(std::shared_ptr<PreviewGenerator> &preview_path);

private:

  void registerEvents() override;

  Ui::PreviewWindow *ui;
  int progress_;
  std::shared_ptr<PreviewGenerator> preview_path_;
};

#endif // PREVIEWWINDOW_H
