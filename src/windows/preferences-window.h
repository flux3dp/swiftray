#pragma once

#include <QDialog>
#include <widgets/base-container.h>

namespace Ui {
  class PreferencesWindow;
}

class PreferencesWindow : public QDialog, BaseContainer {
Q_OBJECT

public:
  explicit PreferencesWindow(QWidget *parent = nullptr);

  ~PreferencesWindow();

  bool isHighSpeedMode();

  bool isUploadEnable();

  void setSpeedMode(bool is_high_speed);

  void setUpload(bool enable_upload);

private:
  Ui::PreferencesWindow *ui;

  void setLanguageComboBox();

  void setSpeedOptimizationComboBox();

  void setShareComboBox();

  void setTabWidget();

  bool is_high_speed_;

  bool is_upload_enable_;

Q_SIGNALS:
  void speedModeChanged(bool is_high_speed);

  void fontSizeChanged(int font_size);

  void privacyUpdate(bool enable_upload);
};
