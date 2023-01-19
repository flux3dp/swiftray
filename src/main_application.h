#ifndef MAIN_APPLICATION_H
#define MAIN_APPLICATION_H

#include <QApplication>
#include <config.h>
#include <constants.h>
#include <QFont>
#include <QList>
#include <QSet>
#include <shape/shape.h>

class MainApplication : public QApplication
{
  Q_OBJECT
public:
  explicit MainApplication(int &argc, char **argv);
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
  double getTransformX();
  double getTransformY();
  double getTransformR();
  double getTransformW();
  double getTransformH();
  bool isShapeScaleLocked();
  //about image
  bool isImageGradient();
  int getImageThreshold();
  //about reference coordinates
  int getJobOrigin();
  int getStartFrom();
  bool getStartWithHome();

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
  //about image
  void updateImageGradient(bool state);
  void updateImageThreshold(int value);
  //about reference coordinates
  void updateReferenceJobOrigin(int job_origin);
  void updateReferenceStartFrom(int start_from);
  void updateReferenceStartWithHome(bool find_home);

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
  //setting of current image
  bool gradient_;
  int thrsh_brightness_;
  //setting of current reference coordinates
  JobOrigin job_origin_;
  StartFrom start_from_;
  bool start_with_home_;

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
  void changeFontEnable(bool enable);
  //about transform
  void editShapeTransform(qreal x, qreal y, qreal r, qreal w, qreal h);
  void editShapeScaleLock(bool locked);
  void changeTransformEnable(bool enable);
  void selectAllGeometry(bool is_geometry);
  void selectAllGroup(bool is_group);
  void changeUnionEnable(bool enable);
  void selectPairPath(bool is_pair);
  void selectGroupEnable(bool enable);
  //about image
  void editImageGradient(bool state);
  void editImageThreshold(int value);
  void changeImageEnable(bool enable);
  //about reference coordinates
  void editReferenceJobOrigin(int job_origin);
  void editReferenceStartFrom(int start_from);
  void editReferenceStartWithHome(bool find_home);
};

extern MainApplication *mainApp;

#endif // MAIN_APPLICATION_H
