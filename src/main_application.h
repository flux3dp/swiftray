#ifndef MAIN_APPLICATION_H
#define MAIN_APPLICATION_H

#include <QApplication>
#include <config.h>
#include <constants.h>
#include <QFont>
#include <QList>
#include <QSet>
#include <shape/shape.h>
#include <settings/machine-settings.h>
#include <settings/rotary-settings.h>

class MainApplication : public QApplication
{
  Q_OBJECT
public:
  explicit MainApplication(int &argc, char **argv);
  ~MainApplication();

  bool isFirstTime();
  bool isUploadEnable();
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
  //about preset
  int getPresetIndex();
  int getParamIndex();
  double getFramingPower();
  double getPulsePower();
  //about machine
  int getMachineIndex();
  bool isHighSpeedMode();
  QSize getWorkingRange();
  char getRotaryAxis();
  double getTravelSpeed();
  MachineSettings::MachineParam getMachineParam();
  void addMachine(MachineSettings::MachineParam new_machine);
  //about rotary
  int getRotaryIndex();
  bool isRotaryMode();
  bool isMirrorMode();
  double getRotaryScale();
  double getRotaryCircumference();
  RotarySettings::RotaryParam getRotaryParam();
  //about canvas
  CanvasQuality getCanvasQuality() { return canvas_quality_; }

public Q_SLOTS:
  void updateUploadEnable(bool enable_upload);
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
  //about preset
  void updatePresetIndex(int preset_index);
  void updatePresetIndex(int preset_index, int param_index);
  void updateParamIndex(int param_index);
  void updateFramingPower(double framing_power);
  void updatePulsePower(double pulse_power);
  //about machine
  void updateMachineIndex(int machine_index);
  void updateMachineRange(QSize machine_size);
  void updateMachineTravelSpeed(double speed);
  void updateMachineRotaryAxis(char axis);
  void updateMachineHighSpeedMode(bool is_high_speed_mode);
  //about rotary
  void updateRotaryIndex(int rotary_index);
  void updateRotaryMode(bool is_rotary_mode);
  void updateMirrorMode(bool is_mirror_mode);
  void updateRotarySpeed(double speed);
  void updateCircumference(double circumference);
  //about canvas
  void updateCanvasQuality(int canvas_quality);

private:
  //about preset
  void initialPreset();
  void savePreset();
  //about machine
  void initialMachine();
  void saveMachine();
  //about rotary
  void initialRotary();
  void calculateRotaryScale();
  void saveRotary();
#if defined(HAVE_SOFTWARE_UPDATE) && defined(Q_OS_WIN)
  bool software_update_ok_ = false;
#endif
  bool is_first_time_;
  bool is_upload_enable_;
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
  //setting of current preset
  int preset_index_;
  int param_index_;//-1 is custom
  //setting of current machine
  int machine_index_;
  double travel_speed_;
  QSize working_range_;
  bool is_high_speed_mode_;
  bool start_with_home_;
  //setting of rotary
  int rotary_index_;
  bool rotary_mode_;
  bool mirror_mode_;
  double rotary_circumference_;
  double rotary_scale_;
  //setting of canvas
  CanvasQuality canvas_quality_;

private Q_SLOTS:
  void cleanup();

Q_SIGNALS:
#if defined(HAVE_SOFTWARE_UPDATE) && defined(Q_OS_WIN)
  // Each of these are called from a separate thread.
  void softwareUpdateRequested();
  void softwareUpdateClose();
  void softwareUpdateQuit();
#endif
  void editUploadEnable(bool enable_upload);
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
  //about preset
  void editPresetIndex(int preset_index, int param_index);
  void editFramingPower(double power);
  void editPulsePower(double power);
  //about machine
  void editMachineIndex(int machine_index);
  void editWorkingRange(QSize machine_size);
  void editMachineTravelSpeed(double speed);
  void editMachineRotaryAxis(char axis);
  void editMachineHighSpeedMode(bool is_high_speed_mode);
  //about rotary
  void editRotaryIndex(int rotary_index);
  void editRotaryMode(bool is_rotary_mode);
  void editRotaryTravelSpeed(double speed);
  void editCircumference(double circumference);
  //about canvas
  void editCanvasQuality(CanvasQuality canvas_quality);
};

extern MainApplication *mainApp;

#endif // MAIN_APPLICATION_H
