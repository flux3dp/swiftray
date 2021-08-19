#pragma once

#ifdef Q_OS_IOS

#include <QObject>
#include <QImage>

class ImagePicker : public QObject {
Q_OBJECT

public:
  static ImagePicker *g_currentImagePicker;

public slots:

  void show(void);

signals:

  void imageSelected(const QImage image);
};

#endif