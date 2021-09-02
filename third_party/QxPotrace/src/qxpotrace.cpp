#include <QxPotrace/include/qxpotrace.h>
#include <potracelib.h>
#include <QDebug>
#include <memory>

#define BM_WORDSIZE ((int)sizeof(potrace_word))
#define BM_WORDBITS (8*BM_WORDSIZE)
#define BM_HIBIT (((potrace_word)1)<<(BM_WORDBITS-1))
#define bm_scanline(bm, y) ((bm)->map + (y)*(bm)->dy)
#define bm_index(bm, x, y) (&bm_scanline(bm, y)[(x)/BM_WORDBITS])
#define bm_mask(x) (BM_HIBIT >> ((x) & (BM_WORDBITS-1)))
#define bm_range(x, a) ((int)(x) >= 0 && (int)(x) < (a))
#define bm_safe(bm, x, y) (bm_range(x, (bm)->w) && bm_range(y, (bm)->h))
#define BM_USET(bm, x, y) (*bm_index(bm, x, y) |= bm_mask(x))
#define BM_UCLR(bm, x, y) (*bm_index(bm, x, y) &= ~bm_mask(x))
#define BM_UPUT(bm, x, y, b) ((b) ? BM_USET(bm, x, y) : BM_UCLR(bm, x, y))
#define BM_PUT(bm, x, y, b) (bm_safe(bm, x, y) ? BM_UPUT(bm, x, y, b) : 0)

/**
 * @param image
 * @param bitmap internal memory block should have been allocated
 * @param cutoff
 * @param threshold
 */
void imageToBinarizedBitmap(const QImage &image, potrace_bitmap_t* bitmap, int cutoff, int threshold)
{
  if (image.isNull()) {
    return;
  }

  bool apply_alpha = image.hasAlphaChannel();
  if (apply_alpha) {
    Q_ASSERT_X(image.format() == QImage::Format_ARGB32, "qxpotrace", "input image must be ARGB32/Grayscale8");
  } else {
    Q_ASSERT_X(image.format() == QImage::Format_Grayscale8, "qxpotrace", "input image must be ARGB32/Grayscale8");
  }

  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      QRgb pixel = image.pixel(x, y);
      int grayscale_val = qGray(pixel);
      int bm_p;
      if (apply_alpha) {
        grayscale_val = 255 - (255 - grayscale_val) * qAlpha(pixel) / 255;
      }
      bm_p = grayscale_val < cutoff ? 0 :
             grayscale_val <= threshold ? 1 : 0;
      BM_PUT(bitmap, x, y, bm_p);
    }
  }
  return;
}

/**
 * @brief Append a closed contour to QPainterPath
 * @param contour
 */
void QxPotrace::convert_potrace_path_to_QPainterPath(potrace_path_t *contour) {
  int num_points = contour->curve.n;
  int *tags = contour->curve.tag;
  potrace_dpoint_t (*points)[3] = contour->curve.c;
  if (num_points > 0) {
    // Move to initial pos of a new contour
    contours_.moveTo(points[num_points-1][2].x, points[num_points-1][2].y);
  }
  for (int i = 0; i < num_points; ++i) {
    switch (tags[i]) {
      case POTRACE_CORNER:
        // sharp corner
        contours_.lineTo(points[i][1].x, points[i][1].y);
        contours_.lineTo(points[i][2].x, points[i][2].y);
        break;

      case POTRACE_CURVETO:
        // Bezier curve
        contours_.cubicTo(points[i][0].x, points[i][0].y,
                         points[i][1].x, points[i][1].y,
                         points[i][2].x, points[i][2].y);
        break;

      default:
        break;
    }
  }
}

void QxPotrace::convert_paths_recursively(potrace_path_s* current_contour_p) {
  try {
    convert_potrace_path_to_QPainterPath(current_contour_p);

    if (current_contour_p->childlist != NULL) {
      convert_paths_recursively(current_contour_p->childlist);
    }
    if (current_contour_p->sibling != NULL) {
      convert_paths_recursively(current_contour_p->sibling);
    }
  } catch (const std::exception& e) {
    qInfo() << e.what();
    return;
  }
}

/**
 * @brief Generate contours of image,
 *        NOTE: All intermediate memory should be freed at the end of the function
 *              Only result contours is left
 * @param image           should be in RGBA or grayscale8 format
 * @param low_thres       equals to cutoff in lightburn
 * @param high_thres      equals to threshold in lightburn
 * @param turd_size       equals to ignore less than in lightburn
 * @param smooth          equals to smoothness in lightburn (larger -> corner round more)
 * @param curve_tolerance equals to optimize in lightburn (higher -> less bezier curve -> simplified path)
 * @return
 */
bool QxPotrace::trace(const QImage &image, int low_thres, int high_thres,
                      int turd_size, qreal smooth, qreal curve_tolerance)
{
  // Allocate and fill bitmap
  auto bitmap = std::make_unique<potrace_bitmap_t>();
  bitmap->h = image.height();
  bitmap->w = image.width();
  bitmap->dy = (image.width() + BM_WORDBITS - 1) / BM_WORDBITS;
  auto bitmap_internal_memblk = std::make_unique<potrace_word[]>(bitmap->dy * bitmap->h);
  bitmap->map = bitmap_internal_memblk.get();

  imageToBinarizedBitmap(image, bitmap.get(), low_thres, high_thres);

  // Set params
  using unique_trace_param_t = std::unique_ptr<potrace_param_t, decltype(&potrace_param_free)>;
  unique_trace_param_t trace_params(
          potrace_param_default(), // return newly allocated param struct
          &potrace_param_free);
  trace_params->alphamax = smooth;
  trace_params->opttolerance = curve_tolerance;
  trace_params->turdsize = turd_size;

  // Start image tracing
  using unique_trace_state_t = std::unique_ptr<potrace_state_t, decltype(&potrace_state_free)>;
  unique_trace_state_t trace_state(
          potrace_trace(trace_params.get(), bitmap.get()), // call libpotrace trace API and return newly allocated state struct
          &potrace_state_free);
  if (!trace_state || trace_state->status != POTRACE_STATUS_OK) {
    return false;
  }

  contours_.clear();
  potrace_path_s* first_contour_node_p = trace_state->plist;
  if (first_contour_node_p != NULL) {
    // convert all contours into a QPainterPath
    convert_paths_recursively(first_contour_node_p);
  }

  return true;
}
