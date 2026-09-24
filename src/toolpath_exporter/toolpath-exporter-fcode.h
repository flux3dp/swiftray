// Updated to
// Ghost: b10a72e5e59c5f03b7e214953557903af141d611
// Client: 1a1e65fd01c55e2a2c1b124bc04543082d7c552c

#pragma once

#include <optional>

#include "toolpath-utils.h"
#include "toolpath-exporter-constants.h"
#include "toolpath_exporter/factories/base-factory.h"
#include "toolpath_exporter/factories/laser-path-filled.h"
#include "toolpath_exporter/factories/laser-raster-galvo.h"
#include "toolpath_exporter/factories/laser-path.h"
#include "toolpath_exporter/generators/fcode-generator.h"
#include "toolpath_exporter/macros/base-macros.h"
#include "layer.h"
#include "shape/bitmap-shape.h"
#include "shape/group-shape.h"
#include "shape/path-shape.h"
#include <QImage>
#include <QJsonArray>
#include <QJsonObject>
#include <QProgressDialog>
#include <QVector>
#include <functional>

enum class ConvertTarget {
  ALL,         // printer and uv
  NON_BITMAP,  // laser first pass
  BITMAP_ONLY  // laser second pass
};

enum class ProgressType {
  PRE_TASK,
  LAYERS,
  PRE_LAYER,
  LAYER_PATH,
  LAYER_BITMAP,
  POST_LAYER,
  POST_TASK
};

struct Config {
  // mm/min
  float min_speed = 3;
  float travel_speed = 7500;  // default val = 7500 in ghost, 12000 in client
  float a_travel_speed = 2000;
  float path_travel_speed =
      7500;  // default val = 3600 for ador, 7500 for others
  float vector_speed_limit = 0;
  float curve_speed_limit = 0;
  // mm^2/s
  float padding_acc = 4000;
  float z_acc = NAN;
  AccelerationData fill_acc = {};
  AccelerationData path_acc = {};
  // mm
  float min_engraving_padding = NAN;
  float min_printing_padding = NAN;
  float spinning_axis_coord_mm = -1;
  float z_offset = 0;
  float loop_compensation = 0;
  float engraving_erode = 0;
  InwardRect workarea_clip = {};
  QPointF diode_offset;
  QPointF job_origin;
  QPointF home_pos;
  QMap<LayerModule, QPointF> module_offsets;
  QRectF prespray;
  // px
  int printing_top_padding = -1;  // use -1 (invalid value) to indicate not set
  int printing_bot_padding = -1;
  int printing_slice_width = -1;
  int printing_slice_height = -1;
  // fountion on/off
  bool enable_pwm = true;
  bool enable_diode = false;
  bool enable_autofocus = false;
  bool enable_custom_backlash = false;
  bool enable_fast_gradient = false;
  bool enable_mock_fast_gradient = false;
  bool enable_multipass_compensation = false;
  bool enable_rotary_z_move = false;
  bool enable_segmentation = false;
  bool enable_s_curve = false;
  bool is_one_way_printing = false;
  bool is_diode_one_way_engraving = false;
  bool is_reverse_engraving = false;
  bool skip_prespray = false;
  bool burst_refresh = false;
  int prespray_times = 3;
  QString color_order;  // 4C ink order by cartridge slot, e.g. "cymk"
  bool use_ga_reorder = true;
  // other
  MachineModules expected_module = MachineModules::NONE;
  float rotary_y_ratio = 1;
  float nozzle_voltage = NAN;
  float nozzle_pulse_width = NAN;
  int watt = 0;
  // --- galvo (HEXA II), see HX2_GALVO_PROTOCOL.md -------------------------
  // Addressable field of the galvo, mm across. 110 mm means +/- 55 mm about the
  // lens centre (§19.4).
  double galvo_field_mm = 110;
  /**
   * The travel a galvo module costs the gantry, when the caller would rather
   * say than let get_boundary's table decide. Already the union over the
   * modules, so it stands for every one of them and is not combined further.
   *
   * Temporary: the values belong to the machine (galvo_work_range) and will
   * come from there once Beam Studio can read them, at which point both ends
   * go back to a fixed number.
   */
  std::optional<InwardRect> galvo_boundary;
  // How far the gantry moves between galvo blocks, mm. Not a free number: it is
  // a whole count of X and Y full steps, so the head can only ever stand on
  // multiples of it and anything in between is unaddressable. Smaller than the
  // field, so neighbouring blocks can reach into each other.
  QSizeF galvo_block_size = QSizeF(100, 100);
  // mm. Total width of the band two neighbouring blocks share, over which a
  // dithered raster thins its dots out so the two add up to one covering.
  // Zero turns the blend off and the seam becomes a hard edge again.
  double galvo_dot_blend_overlap = 10;
  // mm. How far along a scan line a run holds its owner before it can change
  // hands; a dithered image changes site by site instead.
  double galvo_line_blend_segment = 2;
  bool galvo_band_dots = false;
  /** The simulator's processDot block, for the hybrid's band dots. */
  LaserRasterGalvoFactory::Blend::DotProcess galvo_dot_process;
  double galvo_line_blend_overlap = 10;
  LaserRasterGalvoFactory::BlendProfile galvo_line_blend_profile =
      LaserRasterGalvoFactory::BlendProfile::SuperGranular;
  LaserRasterGalvoFactory::RunEmission galvo_line_blend_emission =
      LaserRasterGalvoFactory::RunEmission::Checkerboard;
  LaserRasterGalvoFactory::BlendProfile galvo_dot_blend_profile =
      LaserRasterGalvoFactory::BlendProfile::Granular;
  // mm/min. How fast the gantry moves between galvo blocks. Slower than the
  // ordinary travel speed: a galvo job parks, settles and waits for the sync
  // at every block, so the move is a positioning step rather than a traverse.
  // TODO: provisional, waiting on measurement.
  float galvo_travel_speed = 3000;
  // Where to drop a picture of the tiling, for looking at what the splitter
  // decided. Empty, and nothing is drawn.
  QString galvo_debug_image;
  // Baseline galvo parameters. Everything a list's prologue needs that the
  // layer itself does not carry; see §19.3 and §6.2.
  GalvoParams galvo_params;
  // Split a list once it reaches this many records; 0 leaves lists whole. The
  // protocol sets no upper bound (§19.7).
  int galvo_max_list_commands = 0;
};

class ToolpathExporterFcode : public QObject {
  Q_OBJECT

 public:
  ToolpathExporterFcode(const QJsonObject* param,
                        const QString* thumbnail) noexcept;

  std::string toString();
  void save(QDataStream* out);
  float getTimeCost();
  QJsonObject getMetadata();
  bool convertStack(const QList<LayerPtr>& layers, QProgressDialog* dialog = nullptr);

 Q_SIGNALS:
  void progressChanged(int value);

 public Q_SLOTS:
  void handleCancel();

 private:
  ToolpathProcessor proc;
  std::unique_ptr<BaseBitmapFactory> factory_;
  std::unique_ptr<BaseBitmapFactory> laser_filled_factory_;  // Use another factory to keep filled path data
  std::unique_ptr<LaserPathFactory> laser_path_factory_;
  // Galvo only: filled paths become vector hatch instead of raster (§19.5).
  std::unique_ptr<LaserPathFilledFactory> laser_hatch_factory_;
  // Galvo only: bitmaps are expanded here into marks and dots, for the same
  // reason. Depth bitmaps are held apart because they run a pass per threshold.
  std::unique_ptr<LaserRasterGalvoFactory> laser_raster_factory_;
  // Depth bitmaps run a pass per threshold and are held apart for it.
  std::unique_ptr<LaserRasterGalvoFactory> laser_depth_factory_;
  QVector<const BitmapShape*> galvo_depth_bitmaps_;
  QVector<std::shared_ptr<Workspace>> workspaces_ = {};
  QVector<ShapePtr> laser_bitmaps_;

  // Basic config
  Config config_;
  HardwareType hardware_ = HardwareType::Beambox;
  HardwareProfile hw_profile;
  SupportInfo support_info;
  std::shared_ptr<BaseMacros> macros;
  int magic_number_ = 0;
  bool is_v2_ = false;
  // HEXA II routes laser layers through the galvo board instead of the gantry.
  bool is_galvo_machine_ = false;
  bool is_rotary_task_ = false;
  bool is_3d_task_ = false;
  bool has_job_origin_ = false;
  bool has_printing_task_ = false;
  QSizeF work_area_mm_;
  QTransform transform_base_ = QTransform::fromScale(1.0 / CANVAS_MM_RATIO, 1.0 / CANVAS_MM_RATIO);

  // Updated for each layer
  LayerPtr current_layer_;
  LayerPtr current_layer_2_;
  int layer_repeat_ = 1;
  float layer_speed_;       // mm/min
  float layer_path_speed_;  // mm/min
  float layer_backlash_ = 0;
  float layer_pwm_scale_ = 1;
  bool layer_is_high_quality_ = false;
  bool layer_texture_enabled_ = false;
  LaserTextureParams layer_texture_params_;
  LayerModule layer_module_;
  bool is_laser_layer_ = false;
  bool layer_is_galvo_ = false;
  bool is_printing_layer_ = false;
  bool is_uv_layer_ = false;
  QPointF layer_offset_;
  PrintingColor layer_color_;
  InwardRect layer_clip_;  // mm
  cv::Mat kernel_;

  QJsonArray post_config_;
  int current_layer_id_ = 0;
  LayerModule last_module_ = LayerModule::NONE;
  PrintingColor last_color_;
  QString last_sub_type_;

  // Updated during processing
  QSet<LayerModule> all_modules_;
  ConvertTarget convert_target_ = ConvertTarget::ALL;
  bool did_home_z_ = false;
  QTransform global_transform_;
  LayerModule prespray_module_ = LayerModule::NONE;
  // Task progress
  QProgressDialog* dialog_ = nullptr;
  bool cancelled_ = false;
  int total_layer_cnt_ = 1;
  int processed_layer_cnt_ = 0;
  int total_repeat_times_ = 1;
  int processed_repeat_times_ = 0;
  int element_cnt_[2] = {0, 0};  // 0 Path, 1 Filled Path + Bitmap
  int total_element_cnt_ = 0;
  double bitmap_progress_unit_ = 0;
  double current_progress_ = 0;  // progress within current repeat
  int progress_ = 0;

  void parseParam(const QJsonObject& paramPtr);
  void setTransform(QTransform transform = QTransform());
  /**
   * The travel a module costs the gantry: what the caller stated, or the
   * built-in table. Both the layer clip and the head's own range are cut from
   * this, and they have to be cut from the same thing -- a head allowed to
   * stand where the drawing is not allowed to reach, or the reverse, is a
   * lattice that does not line up with the work it is planning for.
   */
  InwardRect moduleBoundary(LayerModule module) const;
  InwardRect getClipRect(InwardRect current, QPointF offset, LayerModule module, bool rotary = false);
  void onProgressChanged(double value, bool absolute);
  // Layer task
  void convertLayer();
  void preprocessLaserLayer();
  void convertLaserLayer();
  // Galvo counterpart of convertLaserLayer(): parks the head once per tile and
  // writes the geometry as galvo lists (§19).
  void convertGalvoLaserLayer();
  // One head position in the tiling.
  struct GalvoBlock {
    // The tile itself, mm in the layer frame. Its centre is where the head parks.
    QRectF region;
    // `region` with seam ownership settled: a tile keeps its right and bottom
    // edges and, unless it starts a row or column, gives up its left and top
    // ones, so geometry lying along a seam belongs to the tile above or to the
    // left of it rather than being marked by both.
    QRectF clip;
    // Where the gantry stands: a lattice point of the block displacement, which
    // is not the centre of `region` once an edge cell has stretched.
    QPointF park;
    // Position in the lattice, and which sides have a neighbour to share a
    // seam with. Dithered raster needs both: the parity decides which half of
    // the screen this block keeps, and the flags which edges it blends across.
    int col = 0;
    int row = 0;
    bool has_left = false;
    bool has_right = false;
    bool has_top = false;
    bool has_bottom = false;
  };

  // Where the head itself can stand, mm in the layer frame: the work area less
  // the travel this module costs.
  QRectF galvoHeadTravel() const;
  // Tiles `content` (mm, layer frame) into galvo blocks, in the order to run.
  QVector<GalvoBlock> planGalvoBlocks(const QRectF& content) const;
  // Fill the writer's prologue state from the current layer.
  void applyGalvoLayerParams();
  // Turn the layer's bitmaps into greyscale images the raster factory can walk.
  void prepareGalvoBitmaps();
  // Park at each block in turn and let `emit_content` write into it.
  // `centre_when_done` brings the mirrors home at the end of the last block,
  // for when this is the end of the galvo's work.
  void emitGalvoBlocks(const QVector<GalvoBlock>& blocks,
                       const std::function<void()>& emit_content,
                       double progress_from,
                       double progress_to,
                       bool centre_when_done = false);
  // Depth engraving: a pass per threshold, stepping Z between them.
  void convertGalvoDepthBitmaps();
  // Draw what the splitter decided: where the head stands and how far the beam
  // reaches from there. Opt-in via Config::galvo_debug_image.
  void writeGalvoDebugImage(const QVector<GalvoBlock>& blocks,
                            const QRectF& content) const;
  void outputLayerPathFcode();
  void outputBitmapFcode();
  void convertPrintingLayer();
  // Sub task / block
  void outputPrintingTestFcode();
  void writePreviewImage();
  // Shape conversion
  bool convertShape(const ShapePtr& shape, bool from_group = false);
  bool convertGroup(const GroupShape* group);
  void convertBitmap(const BitmapShape* bmp);
  void convertPath(const PathShape* path);
  // Image processing functions
  void clearWhite(QImage* src, QRect dirty_area);
  void clearTransparent(QImage* src);
  void dilateBinaryBitmap(QImage* src);
  void texturizeLaserImage(QImage* src, bool redither);
  // Move related functions
  void homeZAxis();
  void backToHome();
};
