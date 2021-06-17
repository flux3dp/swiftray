#ifndef PREVIEWWINDOW_H
#define PREVIEWWINDOW_H

#include <QDialog>
#include <gcode/generators/preview-generator.h>

namespace Ui {
  class PreviewWindow;
}

class PreviewWindow : public QDialog {
Q_OBJECT

public:
  explicit PreviewWindow(QWidget *parent = nullptr);

  ~PreviewWindow();

  void paintEvent(QPaintEvent *event) override;

  void registerEvents();

private:
  Ui::PreviewWindow *ui;
  int progress_;
  std::shared_ptr<PreviewGenerator> preview_path_;
public:
  PreviewGenerator *previewPath() const;

  void setPreviewPath(std::shared_ptr<PreviewGenerator> &preview_path);
};

#endif // PREVIEWWINDOW_H
