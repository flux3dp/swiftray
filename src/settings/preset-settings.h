#pragma once

#include <QString>
#include <QJsonObject>

// TODO (Redesign logic to PresetSettings -> Preset -> Param)

/**
 * @brief Predefined working params (power & speed) for various laser head and materials
 * 
 */
class PresetSettings : public QObject
{
    Q_OBJECT
public:
  static PresetSettings& getInstance() {
    static PresetSettings sInstance;
    return sInstance;
  }
  struct Param {
  public:
    QString name;
    double power;
    double speed;
    int repeat;
    double step_height;
    double target_height;
    bool use_diode;
    bool use_autofocus;

    Param() :
         name("New Custom Parameter"),
         power(30),
         speed(20),
         repeat(1),
         step_height(0),
         target_height(0),
         use_diode(false),
         use_autofocus(false) {}

    static Param fromJson(const QJsonObject &obj);
    QJsonObject toJson() const;
  };

  struct Preset {
  public:
    QString name;
    double framing_power;
    double pulse_power;
    QList<Param> params;

    Preset() :
         name("New Preset"),
         framing_power(2),
         pulse_power(30) {}

    static Preset fromJson(const QJsonObject &obj);
    QJsonObject toJson() const;
  };

  const QList<Preset> &presets() {
    return presets_;
  }
  const QList<Preset> &getOriginPreset() {
    return origin_preset_;
  }
  void setPresets(const QList<Preset> &new_presets);
  void setOriginPresets(const QList<Preset> &presets);
  void save();
  void reset();
  void loadPreset(const QJsonObject &obj);
  QJsonObject toJson() const;
  Preset getTargetPreset(int preset_index);
  Param getTargetParam(int preset_index, int param_index);
  void setPresetPower(int preset_index, double framing, double pulse);

Q_SIGNALS:
  void savePreset(QJsonObject save_obj);
  void resetPreset();

private:
  PresetSettings();

  QList<Preset> presets_;
  QList<Preset> origin_preset_;
};
