#pragma once

#include <QDialog>
#include <QQuickItem>
#include <QQuickWidget>
#include <settings/machine-settings.h>

class MainWindow;

class WelcomeDialog : public QDialog {
Q_OBJECT
public:
  explicit WelcomeDialog(QWidget *parent);

  ~WelcomeDialog() = default;

signals:

  void settingsChanged();

public slots:

  void createStandardeProfile(QString brand, QString model);

  void createOtherProfile(QString name, int width, int height, int origin);

  void close();

private:
  QQuickWidget *widget_;
};