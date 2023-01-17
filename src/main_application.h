#ifndef MAIN_APPLICATION_H
#define MAIN_APPLICATION_H

#include <QApplication>
#include <config.h>
#include <QFont>
#include <QList>
#include <QSet>
#include <shape/shape.h>

class MainApplication : public QApplication
{
  Q_OBJECT
public:
  explicit MainApplication(int &argc,  char **argv);
  ~MainApplication();

#if defined(HAVE_SOFTWARE_UPDATE) && defined(Q_OS_WIN)
  void rejectSoftwareUpdate() { software_update_ok_ = false; }
  bool softwareUpdateCanShutdown();
  void softwareUpdateShutdownRequest();
#endif

  //about font
  QFont getFont();
  double getFontLineHeight();
  //about transform
  bool isShapeScaleLocked();

public Q_SLOTS:
  void getSelectShapeChange(QList<ShapePtr> shape_list);
  //about font
  void updateShapeFontFamily(QFont font);
  void updateShapeFontPointSize(int point_size);
  void updateShapeLetterSpacing(qreal letter_spacing);
  void updateShapeBold(bool bold);
  void updateShapeItalic(bool italic);
  void updateShapeUnderline(bool underline);
  void updateShapeLineHeight(double line_height);
  //about transform
  void getSelectShapeTransform(qreal x, qreal y, qreal r, qreal w, qreal h);//maybe this should calculate when getSelectShapeChange?(calculate by transform & canvas now)
  void updateShapeTransformX(double x);
  void updateShapeTransformY(double y);
  void updateShapeTransformR(double r);
  void updateShapeTransformW(double w);
  void updateShapeTransformH(double h);
  void updateShapeScaleLock(bool locked);

private:
#if defined(HAVE_SOFTWARE_UPDATE) && defined(Q_OS_WIN)
  bool software_update_ok_ = false;
#endif

  //setting of current font
  QFont font_;
  double line_height_;
  //setting of current transform
  double x_;
  double y_;
  double r_;
  double w_;
  double h_;
  bool scale_locked_;

private Q_SLOTS:
  void cleanup();

Q_SIGNALS:
#if defined(HAVE_SOFTWARE_UPDATE) && defined(Q_OS_WIN)
  // Each of these are called from a separate thread.
  void softwareUpdateRequested();
  void softwareUpdateClose();
  void softwareUpdateQuit();
#endif

  //about font
  void updateFontView(QSet<QString> font_familys, 
                      QSet<int> point_sizes, 
                      QSet<qreal> letter_spacings, 
                      QSet<bool> bolds, 
                      QSet<bool> italics, 
                      QSet<bool> underlines, 
                      QSet<double> line_heights);
  void editShapeFontFamily(QString font_family);
  void editShapeFontPointSize(int point_size);
  void editShapeLetterSpacing(qreal letter_spacing);
  void editShapeBold(bool bold);
  void editShapeItalic(bool italic);
  void editShapeUnderline(bool underline);
  void editShapeLineHeight(double line_height);
  //about transform
  void editShapeTransform(qreal x, qreal y, qreal r, qreal w, qreal h);
  void editShapeScaleLock(bool locked);

};

extern MainApplication *mainApp;

#endif // MAIN_APPLICATION_H
