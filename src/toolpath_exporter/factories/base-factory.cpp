#include "base-factory.h"

BaseFactory::BaseFactory(const FactoryKwargs& kwargs) noexcept
    : proc(kwargs.proc),
      onProgressChanged(kwargs.onProgressChanged),
      offset(kwargs.offset) {
  pixel_per_mm = kwargs.pixel_per_mm.value_or(10);
  pixel_per_mm_x = kwargs.pixel_per_mm_x.value_or(pixel_per_mm);
  qInfo() << "BaseFactory pixel_per_mm:" << pixel_per_mm
          << "pixel_per_mm_x:" << pixel_per_mm_x;
  transform = QTransform::fromScale(pixel_per_mm_x, pixel_per_mm);
  clip_rect.setLeft(kwargs.clip_rect_mm.left * pixel_per_mm_x);
  clip_rect.setTop(kwargs.clip_rect_mm.top * pixel_per_mm);
  clip_rect.setRight((kwargs.work_area_mm.width() - kwargs.clip_rect_mm.right) *
                     pixel_per_mm_x);
  clip_rect.setBottom(
      (kwargs.work_area_mm.height() - kwargs.clip_rect_mm.bottom) *
      pixel_per_mm);
}

void BaseFactory::handleCancel() {
  cancelled = true;
}

QTransform BaseFactory::get_transform() {
  return transform;
}

BaseBitmapFactory::BaseBitmapFactory(const FactoryKwargs& kwargs) noexcept
    : BaseFactory(kwargs),
      one_way(kwargs.one_way),
      split_bbox(kwargs.split_bbox),
      work_area_mm(kwargs.work_area_mm) {
  if (kwargs.workspaces) {
    workspaces = kwargs.workspaces;
  } else {
    workspaces = new QVector<std::shared_ptr<Workspace>>();
    use_own_workspaces = true;
  }
  // Note: Use pixel_per_mm for both x and y during drawing and bitmap conversion and pixel_per_mm_x for x after drawing
  // work_area and clip_rect is recalculated to use pixel_per_mm in Workspace
  transform = QTransform::fromScale(pixel_per_mm, pixel_per_mm);
  work_area = QSize(std::ceil(work_area_mm.width() * pixel_per_mm_x),
                    std::ceil(work_area_mm.height() * pixel_per_mm));
  pixel_size = 1 / pixel_per_mm;
  pixel_size_x = 1 / pixel_per_mm_x;
}

BaseBitmapFactory::~BaseBitmapFactory() {
  if (use_own_workspaces && workspaces) {
    delete workspaces;
    workspaces = nullptr;
  }
}

void BaseBitmapFactory::setup_clip_rect(
    const std::shared_ptr<Workspace>& workspace) {
  if (clip_rect.isNull()) {
    return;
  }
  workspace->set_clip_rect(clip_rect);
}

QPointF BaseBitmapFactory::pixel_to_actual_position(int x, int y) {
  return QPointF(x / pixel_per_mm_x, y / pixel_per_mm);
}

int BaseBitmapFactory::get_padding_pixels(double padding_dist) {
  return qMax(int(padding_dist * pixel_per_mm_x), 10);
}

bool BaseBitmapFactory::is_workspace_valid(int index) {
  if (index < 0) {
    index = default_workspaces_index;
  }
  return is_valid.value(index, false);
}

std::shared_ptr<Workspace> BaseBitmapFactory::get_workspace(int index,
                                                            bool need_setup) {
  if (index < 0) {
    index = default_workspaces_index;
  }
  while (workspaces->size() <= index) {
    workspaces->append(std::make_shared<Workspace>());
  }
  auto& workspace = workspaces->at(index);
  if (need_setup && !is_workspace_valid(index)) {
    workspace->set_size(work_area.width(), work_area.height(), pixel_per_mm_x, pixel_per_mm);
    setup_clip_rect(workspace);
    if (is_valid.size() <= index) {
      is_valid.resize(index + 1);
    }
    is_valid[index] = true;
  }
  return workspace;
}

void BaseBitmapFactory::add_image(QImage& img, QRectF& bbox, int index) {
  if (index < 0) {
    index = default_workspaces_index;
  }
  get_workspace(index, true)->add_image(img, bbox);
}
