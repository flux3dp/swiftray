#include "main_application.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <utils/software_update.h>
#include <shape/bitmap-shape.h>
#include <shape/text-shape.h>
#include <settings/preset-settings.h>

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include "server/swiftray-server.h"


MainApplication *mainApp = NULL;

MainApplication::MainApplication(int &argc,  char **argv) :
    QApplication(argc, argv)
{
  mainApp = this;
  QSettings settings("flux", "swiftray");
  QVariant upload_code = settings.value("window/upload", 0);
  is_upload_enable_ = upload_code.toBool();
  QVariant newstart_code = settings.value("window/newstart");
  is_first_time_ = false;
  if(!newstart_code.toInt()) {
    is_first_time_ = true;
  }
  settings.setValue("window/newstart", 1);

#if defined(HAVE_SOFTWARE_UPDATE) && defined(Q_OS_WIN)
  connect(this, &MainApplication::softwareUpdateQuit, this, &QApplication::quit, Qt::QueuedConnection);
#endif

  font_ = QFont(FONT_TYPE, FONT_SIZE, QFont::Bold);
  line_height_ = LINE_HEIGHT;
  x_ = y_ = r_ = w_ = h_ = 0;
  scale_locked_ = false;
  QVariant reference_code = settings.value("window/reference", 0);
  reference_origin_ = (JobOrigin)reference_code.toInt();
  gradient_ = Qt::Checked;
  thrsh_brightness_ = 128;
  job_origin_ = NW;
  start_from_ = AbsoluteCoords;
  start_with_home_ = true;
  initialPreset();
  rotary_mode_ = false;
  initialMachine();
  rotary_scale_ = 1;
  initialRotary();

  QVariant quality_code = settings.value("canvas/quality", 0);
  canvas_quality_ = (CanvasQuality)quality_code.toInt();
  QVariant sort_code = settings.value("canvas/generating_rule", NestedSort);
  path_sort_ = (PathSort)sort_code.toInt();
  swiftray_server_ = new SwiftrayServer(6611);
  // NOTE: qApp: built-in macro of the QApplication
  connect(qApp, &QApplication::aboutToQuit, this, &MainApplication::cleanup);

}

void MainApplication::cleanup()
{
    // TODO: Currently most of the app init/cleanup/actions are handled in MainWindow, 
    //       to be moved to MainApplication in the future
    //software_update_cleanup(); 
    // Write the user's recent file(s) to disk.
    //write_profile_recent();
    //write_recent();

    // We might end up here via exit_application.
    //QThreadPool::globalInstance()->waitForDone();
}

MainApplication::~MainApplication()
{
    mainApp = NULL;
}

bool MainApplication::isUploadEnable() {
  return is_upload_enable_;
}

bool MainApplication::isFirstTime() {
  if(is_first_time_) {
    is_first_time_ = false;
    return true;
  }
  return false;
}

#if defined(HAVE_SOFTWARE_UPDATE) && defined(Q_OS_WIN)
bool MainApplication::softwareUpdateCanShutdown() {
  software_update_ok_ = true;
  // At this point the update is ready to install, but WinSparkle has
  // not yet run the installer. We need to close our "Wireshark is
  // running" mutexes along with those of our child processes, e.g.
  // dumpcap.

  // Step 1: See if we have any open files.
  Q_EMIT softwareUpdateRequested();
  if (software_update_ok_ == true) {

    // Step 2: Close the "running" mutexes.
    //Q_EMIT softwareUpdateClose();
    //close_app_running_mutex();
  }
  return software_update_ok_;
}

void MainApplication::softwareUpdateShutdownRequest() {
  // At this point the installer has been launched. Neither Wireshark nor
  // its children should have any "Wireshark is running" mutexes open.
  // The main window should be closed.

  // Step 3: Quit.
  Q_EMIT softwareUpdateQuit();
}

/** Check to see if Wireshark can shut down safely (e.g. offer to save the
 *  current capture).
 */
extern "C" int software_update_can_shutdown_callback(void) {
  return mainApp->softwareUpdateCanShutdown();
}

/** Shut down Wireshark in preparation for an upgrade.
 */
extern "C" void software_update_shutdown_request_callback(void) {
  mainApp->softwareUpdateShutdownRequest();
}
#endif // HAVE_SOFTWARE_UPDATE && Q_OS_WIN

void MainApplication::updateUploadEnable(bool enable_upload) {
  is_upload_enable_ = enable_upload;
  QSettings settings("flux", "swiftray");
  settings.setValue("window/upload", is_upload_enable_);
  Q_EMIT editUploadEnable(is_upload_enable_);
}

void MainApplication::getSelectShapeChange(QList<ShapePtr> shape_list) {
  bool all_path = !shape_list.empty();
  bool all_group = !shape_list.empty();
  bool all_geometry = !shape_list.empty();
  bool has_txt = false;
  //image enable
  if(shape_list.size() == 1 && shape_list.at(0)->type() == ::Shape::Type::Bitmap) {
    BitmapShape* selected_img = dynamic_cast<BitmapShape *>(shape_list.at(0).get());
    gradient_ = selected_img->gradient();
    thrsh_brightness_ = selected_img->thrsh_brightness();
    Q_EMIT editImageGradient(gradient_);
    Q_EMIT editImageThreshold(thrsh_brightness_);
    Q_EMIT changeImageEnable(true);
  } else {
    Q_EMIT changeImageEnable(false);
  }
  //transform enable
  if(shape_list.empty()) {
    Q_EMIT changeTransformEnable(false);
  } else {
    Q_EMIT changeTransformEnable(true);
  }
  QSet<QString> font_family;
  QSet<int> point_size;
  QSet<qreal> letter_spacing;
  QSet<bool> bold;
  QSet<bool> italic;
  QSet<bool> underline;
  QSet<double> line_height;
  for(auto &shape : shape_list) {
    if(shape->type() != Shape::Type::Path && shape->type() != Shape::Type::Text) all_path = false;
    if(shape->type() != Shape::Type::Group) all_group = false;
    if(shape->type() != Shape::Type::Path) all_geometry = false;
    if(shape->type() == ::Shape::Type::Text) {
      TextShape *txt_shape = (TextShape*)shape.get();
      has_txt = true;
      font_family.insert(txt_shape->font().family());
      point_size.insert(txt_shape->font().pointSize());
      letter_spacing.insert(txt_shape->font().letterSpacing());
      bold.insert(txt_shape->font().bold());
      italic.insert(txt_shape->font().italic());
      underline.insert(txt_shape->font().underline());
      line_height.insert(txt_shape->lineHeight());
    }
  }
  if(has_txt) {
    if(font_family.size() == 1) {
      font_.setFamily(*font_family.begin());
    }
    if(point_size.size() == 1) {
      font_.setPointSize(*point_size.begin());
    }
    if(letter_spacing.size() == 1) {
      font_.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, *letter_spacing.begin());
    }
    if(bold.size() == 1) {
      font_.setBold(*bold.begin());
    }
    if(italic.size() == 1) {
      font_.setItalic(*italic.begin());
    }
    if(underline.size() == 1) {
      font_.setUnderline(*underline.begin());
    }
    if(line_height.size() == 1) {
      line_height_ = *line_height.begin();
    }
    Q_EMIT changeFontEnable(true);
    Q_EMIT updateFontView(font_family, point_size, letter_spacing, bold, 
                          italic, underline, line_height);
  } else {
    font_family.insert(font_.family());
    point_size.insert(font_.pointSize());
    letter_spacing.insert(font_.letterSpacing());
    bold.insert(font_.bold());
    italic.insert(font_.italic());
    underline.insert(font_.underline());
    line_height.insert(line_height_);
    Q_EMIT changeFontEnable(false);
    Q_EMIT updateFontView(font_family, point_size, letter_spacing, bold, 
                          italic, underline, line_height);
  }
  Q_EMIT selectAllGeometry(all_geometry);
  Q_EMIT selectAllGroup(all_group);
  Q_EMIT changeUnionEnable(shape_list.size() > 1 && all_path);
  Q_EMIT selectPairPath(shape_list.size() == 2 && all_path);
  Q_EMIT selectGroupEnable(shape_list.size() > 1);
}

//about font
QFont MainApplication::getFont() {
  return font_;
}

double MainApplication::getFontLineHeight() {
  return line_height_;
}

void MainApplication::updateShapeFontFamily(QFont font) {
  font_.setFamily(font.family());
  Q_EMIT editShapeFontFamily(font.family());
}

void MainApplication::updateShapeFontPointSize(int point_size) {
  font_.setPointSize(point_size);
  Q_EMIT editShapeFontPointSize(point_size);
}

void MainApplication::updateShapeLetterSpacing(qreal letter_spacing) {
  font_.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, letter_spacing);
  Q_EMIT editShapeLetterSpacing(letter_spacing);
}

void MainApplication::updateShapeBold(bool bold) {
  font_.setBold(bold);
  Q_EMIT editShapeBold(bold);
}

void MainApplication::updateShapeItalic(bool italic) {
  font_.setItalic(italic);
  Q_EMIT editShapeItalic(italic);
}

void MainApplication::updateShapeUnderline(bool underline) {
  font_.setUnderline(underline);
  Q_EMIT editShapeUnderline(underline);
}

void MainApplication::updateShapeLineHeight(double line_height) {
  line_height_ = line_height;
  Q_EMIT editShapeLineHeight(line_height);
}

//about transform
double MainApplication::getTransformX() {
  return x_;
}

double MainApplication::getTransformY() {
  return y_;
}

double MainApplication::getTransformR() {
  return r_;
}

double MainApplication::getTransformW() {
  return w_;
}

double MainApplication::getTransformH() {
  return h_;
}

bool MainApplication::isShapeScaleLocked() {
  return scale_locked_;
}

int MainApplication::getShapeReference() {
  return reference_origin_;
}

void MainApplication::getSelectShapeTransform(qreal x, qreal y, qreal r, qreal w, qreal h) {
  x_ = x / 10;
  y_ = y / 10;
  r_ = r;
  w_ = w / 10;
  h_ = h / 10;
  Q_EMIT editShapeTransform(x_, y_, r_, w_, h_);
}

void MainApplication::updateShapeTransformX(double x) {
  if(x_ != x) {
    x_ = x;
    Q_EMIT editShapeTransform(x_, y_, r_, w_, h_);
  }
}

void MainApplication::updateShapeTransformY(double y) {
  if(y_ != y) {
    y_ = y;
    Q_EMIT editShapeTransform(x_, y_, r_, w_, h_);
  }
}

void MainApplication::updateShapeTransformR(double r) {
  if(r_ != r) {
    r_ = r;
    Q_EMIT editShapeTransform(x_, y_, r_, w_, h_);
  }
}

void MainApplication::updateShapeTransformW(double w) {
  if(w == 0) {
    Q_EMIT editShapeTransform(x_, y_, r_, w_, h_);
    return;
  }
  if(w_ != w) {
    if (scale_locked_) {
      h_ = w_ == 0 ? 0 : h_ * w / w_;
    }
    w_ = w;
    Q_EMIT editShapeTransform(x_, y_, r_, w_, h_);
  }
}

void MainApplication::updateShapeTransformH(double h) {
  if(h == 0) {
    Q_EMIT editShapeTransform(x_, y_, r_, w_, h_);
    return;
  }
  if(h_ != h) {
    if (scale_locked_) {
      w_ = h_ == 0 ? 0 : w_ * h / h_;
    }
    h_ = h;
    Q_EMIT editShapeTransform(x_, y_, r_, w_, h_);
  }
}

void MainApplication::updateShapeScaleLock(bool locked) {
  scale_locked_ = locked;
  Q_EMIT editShapeScaleLock(scale_locked_);
}

void MainApplication::updateShapeReference(int reference_origin) {
  reference_origin_ = (JobOrigin)reference_origin;
  QSettings settings("flux", "swiftray");
  settings.setValue("window/reference", reference_origin);
  Q_EMIT editShapeReference(reference_origin_);
}

//about image
bool MainApplication::isImageGradient() {
  return gradient_;
}

int MainApplication::getImageThreshold() {
  return thrsh_brightness_;
}

void MainApplication::updateImageGradient(bool state) {
  gradient_ = state;
  Q_EMIT editImageGradient(gradient_);
}

void MainApplication::updateImageThreshold(int value) {
  thrsh_brightness_ = value;
  Q_EMIT editImageThreshold(thrsh_brightness_);
}

//about reference coordinates
int MainApplication::getJobOrigin() {
  return job_origin_;
}

int MainApplication::getStartFrom() {
  return start_from_;
}

bool MainApplication::getStartWithHome() {
  return start_with_home_;
}

void MainApplication::updateReferenceJobOrigin(int job_origin) {
  if(job_origin >= TotalJobOrigin) return;
  job_origin_ = (JobOrigin)job_origin;
  Q_EMIT editReferenceJobOrigin(job_origin_);
}

void MainApplication::updateReferenceStartFrom(int start_from) {
  if(start_from >= TotalStartFrom) return;
  start_from_ = (StartFrom)start_from;
  Q_EMIT editReferenceStartFrom(start_from_);
}

void MainApplication::updateReferenceStartWithHome(bool find_home) {
  start_with_home_ = find_home;
  MachineSettings* machine_settings = &MachineSettings::getInstance();
  machine_settings->setMachineStartWithHome(machine_index_, start_with_home_);
  saveMachine();
  Q_EMIT editReferenceStartWithHome(start_with_home_);
}

//about preset
void MainApplication::initialPreset() {
  PresetSettings* preset_settings = &PresetSettings::getInstance();
  QList<PresetSettings::Preset> origin_preset;
  QList<QString> file_list;
  file_list.append("1.6W.json");
  file_list.append("5W.json");
  file_list.append("10W.json");
  file_list.append("20W-fiber.json");
  for (int i = 0; i < file_list.size(); ++i) {
    QFile file(":/resources/parameters/"+file_list[i]);
    file.open(QFile::ReadOnly);
    // TODO (Is it possible to remove QJsonDocument and use QJsonObject only?)
    auto file_json = QJsonDocument::fromJson(file.readAll()).object();
    auto preset = PresetSettings::Preset::fromJson(file_json);
    origin_preset << preset;
    if (preset.name.indexOf("10W") > -1) {
      preset_index_ = i;
    }
  }
  preset_settings->setOriginPresets(origin_preset);
  
  QSettings settings("flux", "swiftray");
  QJsonObject obj = settings.value("preset/user").toJsonObject();
  if (obj["data"].isNull()) {
    preset_settings->setPresets(origin_preset);
    savePreset();
    settings.setValue("preset/index", preset_index_);
  } else {
    preset_settings->loadPreset(obj);
    QVariant old_index = settings.value("preset/index", 0);
    if(!old_index.isNull() && old_index.toInt() < preset_settings->presets().size()) {
      preset_index_ = old_index.toInt();
    } else {
      settings.setValue("preset/index", preset_index_);
    }
  }
  connect(preset_settings, &PresetSettings::savePreset, [=](QJsonObject save_obj) {
    savePreset();
  });
  connect(preset_settings, &PresetSettings::resetPreset, [=]() {
    savePreset();
    param_index_ = -1;
  });
  param_index_ = -1;
}

int MainApplication::getPresetIndex() {
  return preset_index_;
}

int MainApplication::getParamIndex() {
  return param_index_;
}

double MainApplication::getFramingPower() {
  PresetSettings* preset_settings = &PresetSettings::getInstance();
  PresetSettings::Preset current_preset = preset_settings->getTargetPreset(preset_index_);
  return current_preset.framing_power;
}

double MainApplication::getPulsePower() {
  PresetSettings* preset_settings = &PresetSettings::getInstance();
  PresetSettings::Preset current_preset = preset_settings->getTargetPreset(preset_index_);
  return current_preset.pulse_power;
}

void MainApplication::updatePresetIndex(int preset_index) {
  updatePresetIndex(preset_index, param_index_);
}

void MainApplication::updatePresetIndex(int preset_index, int param_index) {
  PresetSettings* preset_settings = &PresetSettings::getInstance();
  if(preset_index < preset_settings->presets().size()) {
    preset_index_ = preset_index;
    QSettings settings("flux", "swiftray");
    settings.setValue("preset/index", preset_index_);
    PresetSettings::Preset current_preset = preset_settings->getTargetPreset(preset_index_);
    Q_EMIT editFramingPower(current_preset.framing_power);
    Q_EMIT editPulsePower(current_preset.pulse_power);
    if(param_index < preset_settings->getTargetPreset(preset_index_).params.size()) {
      param_index_ = param_index;
    } else {
      param_index_ = -1;
    }
  }
  Q_EMIT editPresetIndex(preset_index_, param_index_);
}

void MainApplication::updateParamIndex(int param_index) {
  updatePresetIndex(preset_index_, param_index);
}

void MainApplication::updateFramingPower(double framing_power) {
  PresetSettings* preset_settings = &PresetSettings::getInstance();
  PresetSettings::Preset current_preset = preset_settings->getTargetPreset(preset_index_);
  preset_settings->setPresetPower(preset_index_, framing_power, current_preset.pulse_power);
  savePreset();
  Q_EMIT editFramingPower(framing_power);
}

void MainApplication::updatePulsePower(double pulse_power) {
  PresetSettings* preset_settings = &PresetSettings::getInstance();
  PresetSettings::Preset current_preset = preset_settings->getTargetPreset(preset_index_);
  preset_settings->setPresetPower(preset_index_, current_preset.framing_power, pulse_power);
  savePreset();
  Q_EMIT editPulsePower(pulse_power);
}

void MainApplication::savePreset() {
  PresetSettings* preset_settings = &PresetSettings::getInstance();
  QSettings settings("flux", "swiftray");
  settings.setValue("preset/user", preset_settings->toJson());
}

//about machine
void MainApplication::initialMachine() {
  MachineSettings* machine_settings = &MachineSettings::getInstance();
  QSettings settings("flux", "swiftray");
  QJsonObject obj = settings.value("machines/machines").toJsonObject();
  if (obj["data"].isNull()) {
    MachineSettings::MachineParam machine = MachineSettings::findPreset("Lazervida", "Lazervida");
    machine.name = "Lazervida";
    machine.is_high_speed_mode = true;
    machine_settings->addMachine(machine);
    saveMachine();
    machine_index_ = 0;
    settings.setValue("machines/index", machine_index_);
    MachineSettings::MachineParam machine2 = MachineSettings::findPreset("Promark", "Promark");
    machine2.name = "Promark";
    machine2.is_high_speed_mode = false;
    machine_settings->addMachine(machine2);
    saveMachine();
    machine_index_ = 1;
    settings.setValue("machines/index", machine_index_);
  } else {
    machine_settings->loadJson(obj);
    QVariant old_index = settings.value("machines/index", 0);
    if(!old_index.isNull() && old_index.toInt() < machine_settings->getMachines().size()) {
      machine_index_ = old_index.toInt();
    } else {
      machine_index_ = 0;
      settings.setValue("machines/index", machine_index_);
    }
  }
  connect(machine_settings, &MachineSettings::saveMachines, [=](QJsonObject save_obj) {
    saveMachine();
  });
  MachineSettings::MachineParam current_machine = machine_settings->getTargetMachine(machine_index_);
  travel_speed_ = current_machine.travel_speed;
  working_range_ = QSize(current_machine.width, current_machine.height);
  is_high_speed_mode_ = current_machine.is_high_speed_mode;
  start_with_home_ = current_machine.home_on_start;
}

int MainApplication::getMachineIndex() {
  return machine_index_;
}

bool MainApplication::isHighSpeedMode() {
  return is_high_speed_mode_;
}

QSize MainApplication::getWorkingRange() {
  return working_range_;
}

char MainApplication::getRotaryAxis() {
  MachineSettings* machine_settings = &MachineSettings::getInstance();
  MachineSettings::MachineParam current_machine = machine_settings->getTargetMachine(machine_index_);
  return current_machine.rotary_axis;
}

double MainApplication::getTravelSpeed() {
  return travel_speed_;
}

MachineSettings::MachineParam MainApplication::getMachineParam() {
  MachineSettings* machine_settings = &MachineSettings::getInstance();
  return machine_settings->getTargetMachine(machine_index_);
}

void MainApplication::addMachine(MachineSettings::MachineParam new_machine) {
  MachineSettings* machine_settings = &MachineSettings::getInstance();
  machine_settings->addMachine(new_machine);
  machine_index_ = machine_settings->getMachines().size()-1;
  saveMachine();
  Q_EMIT editMachineIndex(machine_index_);
}

void MainApplication::updateMachineIndex(int machine_index) {
  MachineSettings* machine_settings = &MachineSettings::getInstance();
  if(machine_index < machine_settings->getMachines().size()) {
    machine_index_ = machine_index;
    QSettings settings("flux", "swiftray");
    settings.setValue("machines/index", machine_index_);
    MachineSettings::MachineParam current_machine = machine_settings->getTargetMachine(machine_index_);
    if(!rotary_mode_) {
      travel_speed_ = current_machine.travel_speed;
      working_range_ = QSize(current_machine.width, current_machine.height);
    } else {
      working_range_ = QSize(current_machine.width, rotary_circumference_);
    }
    if(is_high_speed_mode_ != current_machine.is_high_speed_mode) {
      is_high_speed_mode_ = current_machine.is_high_speed_mode;
      Q_EMIT editMachineHighSpeedMode(is_high_speed_mode_);
    }
    Q_EMIT editWorkingRange(working_range_);
    Q_EMIT editMachineTravelSpeed(current_machine.travel_speed);
    Q_EMIT editMachineRotaryAxis(current_machine.rotary_axis);
    Q_EMIT editReferenceStartWithHome(current_machine.home_on_start);
  }
  Q_EMIT editMachineIndex(machine_index_);
}

void MainApplication::updateMachineRange(QSize machine_size) {
  MachineSettings* machine_settings = &MachineSettings::getInstance();
  if(machine_size.width() > 0) {
    machine_settings->setMachineRange(machine_index_, machine_size.width(), machine_size.height());
    saveMachine();
  }
  MachineSettings::MachineParam current_machine = machine_settings->getTargetMachine(machine_index_);
  if(!rotary_mode_) {
    working_range_ = QSize(current_machine.width, current_machine.height);
  } else {
    working_range_ = QSize(current_machine.width, rotary_circumference_);
  }
  Q_EMIT editWorkingRange(working_range_);
}

void MainApplication::updateMachineTravelSpeed(double speed) {
  MachineSettings* machine_settings = &MachineSettings::getInstance();
  if(speed > 0) {
    if(!rotary_mode_) {
      travel_speed_ = speed;
    }
    machine_settings->setMachineTravelSpeed(machine_index_, speed);
    saveMachine();
  }
  MachineSettings::MachineParam current_machine = machine_settings->getTargetMachine(machine_index_);
  Q_EMIT editMachineTravelSpeed(current_machine.travel_speed);
}

void MainApplication::updateMachineRotaryAxis(char axis) {
  MachineSettings* machine_settings = &MachineSettings::getInstance();
  switch (axis) {
    case 'Y':
    case 'Z':
    case 'A':
      machine_settings->setMachineRotaryAxis(machine_index_, axis);
      saveMachine();
      break;
    default:
      qInfo() << Q_FUNC_INFO << " get axis: " << axis << " over range";
      break;
  }
  MachineSettings::MachineParam current_machine = machine_settings->getTargetMachine(machine_index_);
  Q_EMIT editMachineRotaryAxis(current_machine.rotary_axis);
}

void MainApplication::updateMachineHighSpeedMode(bool is_high_speed_mode) {
  MachineSettings* machine_settings = &MachineSettings::getInstance();
  machine_settings->setMachineHightSpeedMode(machine_index_, is_high_speed_mode);
  saveMachine();
  is_high_speed_mode_ = is_high_speed_mode;
  Q_EMIT editMachineHighSpeedMode(is_high_speed_mode_);
}

void MainApplication::saveMachine() {
  MachineSettings* machine_settings = &MachineSettings::getInstance();
  QSettings settings("flux", "swiftray");
  settings.setValue("machines/machines", machine_settings->toJson());
}

//about rotary
void MainApplication::initialRotary() {
  mirror_mode_ = false;
  RotarySettings* rotary_settings = &RotarySettings::getInstance();
  QSettings settings("flux", "swiftray");
  QJsonObject obj = settings.value("rotarys/rotarys").toJsonObject();
  if (obj["data"].isNull()) {
    RotarySettings::RotaryParam rotary;
    rotary.name = "Lazervida Rotary";
    rotary_settings->addRotary(rotary);
    saveRotary();
    rotary_index_ = 0;
    settings.setValue("rotarys/index", rotary_index_);
    rotary_circumference_ = working_range_.height();
    settings.setValue("rotarys/circumference", rotary_circumference_);
  } else {
    rotary_settings->loadRotary(obj);
    QVariant old_index = settings.value("rotarys/index", 0);
    if(!old_index.isNull() && old_index.toInt() < rotary_settings->rotarys().size()) {
      rotary_index_ = old_index.toInt();
    } else {
      rotary_index_ = 0;
      settings.setValue("rotarys/index", rotary_index_);
    }
    QVariant old_circumference = settings.value("rotarys/circumference", 0);
    if(!old_circumference.isNull()) {
      rotary_circumference_ = old_circumference.toDouble();
    } else {
      rotary_circumference_ = working_range_.height();
      settings.setValue("rotarys/circumference", rotary_circumference_);
    }
  }
  connect(rotary_settings, &RotarySettings::saveRotary, [=](QJsonObject save_obj) {
    saveRotary();
  });
}

int MainApplication::getRotaryIndex() {
  return rotary_index_;
}

bool MainApplication::isRotaryMode() {
  return rotary_mode_;
}

bool MainApplication::isMirrorMode() {
  if(!rotary_mode_) return false;
  return mirror_mode_;
}

double MainApplication::getRotaryScale() {
  return rotary_scale_;
}

double MainApplication::getRotaryCircumference() {
  return rotary_circumference_;
}

RotarySettings::RotaryParam MainApplication::getRotaryParam() {
  RotarySettings* rotary_settings = &RotarySettings::getInstance();
  return rotary_settings->getTargetRotary(rotary_index_);
}

void MainApplication::updateRotaryIndex(int rotary_index) {
  RotarySettings* rotary_settings = &RotarySettings::getInstance();
  if(rotary_index < rotary_settings->rotarys().size()) {
    rotary_index_ = rotary_index;
    RotarySettings::RotaryParam current_rotary = rotary_settings->getTargetRotary(rotary_index_);
    QSettings settings("flux", "swiftray");
    settings.setValue("rotarys/index", rotary_index_);
    if(rotary_mode_) travel_speed_ = current_rotary.travel_speed;
    Q_EMIT editRotaryTravelSpeed(current_rotary.travel_speed);
  }
  calculateRotaryScale();
}

void MainApplication::updateRotaryMode(bool is_rotary_mode) {
  MachineSettings* machine_settings = &MachineSettings::getInstance();
  MachineSettings::MachineParam current_machine = machine_settings->getTargetMachine(machine_index_);
  RotarySettings* rotary_settings = &RotarySettings::getInstance();
  RotarySettings::RotaryParam current_rotary = rotary_settings->getTargetRotary(rotary_index_);
  rotary_mode_ = is_rotary_mode;
  if(rotary_mode_) {
    working_range_ = QSize(current_machine.width, rotary_circumference_);
    travel_speed_ = current_rotary.travel_speed;
    Q_EMIT editRotaryTravelSpeed(current_rotary.travel_speed);
    calculateRotaryScale();
  } else {
    working_range_ = QSize(current_machine.width, current_machine.height);
    travel_speed_ = current_machine.travel_speed;
    Q_EMIT editMachineTravelSpeed(current_machine.travel_speed);
  }
  Q_EMIT editWorkingRange(working_range_);
  Q_EMIT editRotaryMode(rotary_mode_);
}

void MainApplication::updateMirrorMode(bool is_mirror_mode) {
  mirror_mode_ = is_mirror_mode;
}

void MainApplication::updateRotarySpeed(double speed) {
  RotarySettings* rotary_settings = &RotarySettings::getInstance();
  if(speed > 0) {
    if(rotary_mode_) {
      travel_speed_ = speed;
    }
    rotary_settings->setRotarySpeed(rotary_index_, speed);
    saveRotary();
  }
  RotarySettings::RotaryParam current_rotary = rotary_settings->getTargetRotary(rotary_index_);
  Q_EMIT editRotaryTravelSpeed(current_rotary.travel_speed);
}

void MainApplication::updateCircumference(double circumference) {
  MachineSettings* machine_settings = &MachineSettings::getInstance();
  MachineSettings::MachineParam current_machine = machine_settings->getTargetMachine(machine_index_);
  if(circumference > 0) {
    rotary_circumference_ = circumference;
    QSettings settings("flux", "swiftray");
    settings.setValue("rotarys/circumference", rotary_circumference_);
    if(rotary_mode_) {
      working_range_ = QSize(current_machine.width, rotary_circumference_);
      Q_EMIT editWorkingRange(working_range_);
    }
  }
  calculateRotaryScale();
  Q_EMIT editCircumference(rotary_circumference_);
}

void MainApplication::calculateRotaryScale() {
  RotarySettings* rotary_settings = &RotarySettings::getInstance();
  RotarySettings::RotaryParam current_rotary = rotary_settings->getTargetRotary(rotary_index_);
  if(current_rotary.rotary_type == RotarySettings::RotaryType::Roller) {
    double circumference = current_rotary.roller_diameter * M_PI;
    if(circumference > 0) {
      rotary_scale_ = current_rotary.mm_per_rotation / circumference;
    }
    else {
      rotary_scale_ = 0;
    }
  } else {
    if(rotary_circumference_ > 0) {
      rotary_scale_ = current_rotary.mm_per_rotation / rotary_circumference_;
    }
    else {
      rotary_scale_ = 0;
    }
  }
}

void MainApplication::saveRotary() {
  RotarySettings* rotary_settings = &RotarySettings::getInstance();
  QSettings settings("flux", "swiftray");
  settings.setValue("rotarys/rotarys", rotary_settings->toJson());
}

//about canvas
void MainApplication::updateCanvasQuality(int canvas_quality) {
  canvas_quality_ = (CanvasQuality)canvas_quality;
  QSettings settings("flux", "swiftray");
  settings.setValue("canvas/quality", canvas_quality);
  Q_EMIT editCanvasQuality(canvas_quality_);
}

//about tool path
void MainApplication::updatePathSort(int path_sort) {
  path_sort_ = (PathSort)path_sort;
  QSettings settings("flux", "swiftray");
  settings.setValue("canvas/generating_rule", path_sort);
  Q_EMIT editPathSort(path_sort_);
}