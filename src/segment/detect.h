// detect.h - "segment everything" detector: prompt grid + brightness-blob
// candidates -> SAM decode -> filters + NMS -> objects with centers and
// polygon contours. Ported from mini-sam/src/detect.h (validated against the
// Python prototype); classic-CV blocks are kept hand-rolled for parity.
#pragma once
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <thread>
#include <vector>

#include "sam.h"

namespace segment {

struct DetectedObject {
    std::vector<uint8_t> mask;            // full-frame binary
    float cx = 0, cy = 0;                 // centroid (all mask pixels)
    int area = 0;                         // mask pixel count
    float score = 0;                      // SAM iou prediction
    float stability = 0;                  // mask area at logit>1 / area at logit>-1 (filter diagnostics)
    float border = 0;                     // share of the image border band the low-res mask touches
    int bx = 0, by = 0, bw = 0, bh = 0;   // bbox
    std::vector<std::pair<float, float>> polygon;  // simplified outer contour
};

// ---- classic CV building blocks (hand-rolled, no OpenCV) ----

inline std::vector<uint8_t> to_gray(const Image& img) {
    std::vector<uint8_t> g((size_t)img.w * img.h);
    for (size_t i = 0; i < g.size(); i++) {
        const uint8_t* p = &img.px[i * 3];  // RGB
        g[i] = (uint8_t)((p[0] * 299 + p[1] * 587 + p[2] * 114) / 1000);
    }
    return g;
}

inline uint8_t otsu_threshold(const std::vector<uint8_t>& gray) {
    int hist[256] = {0};
    for (uint8_t v : gray) hist[v]++;
    double total = (double)gray.size(), sum = 0;
    for (int i = 0; i < 256; i++) sum += (double)i * hist[i];
    double sum_b = 0, w_b = 0, max_var = 0;
    int thresh = 127;
    for (int t = 0; t < 256; t++) {
        w_b += hist[t];
        if (w_b == 0) continue;
        double w_f = total - w_b;
        if (w_f == 0) break;
        sum_b += (double)t * hist[t];
        double m_b = sum_b / w_b, m_f = (sum - sum_b) / w_f;
        double var = w_b * w_f * (m_b - m_f) * (m_b - m_f);
        if (var > max_var) { max_var = var; thresh = t; }
    }
    return (uint8_t)thresh;
}

// separable square-kernel morphology (erode = min, dilate = max)
inline void morph_1d(std::vector<uint8_t>& img, int w, int h, int radius, bool horizontal, bool is_max) {
    std::vector<uint8_t> out(img.size());
    int len = horizontal ? w : h, lines = horizontal ? h : w;
    for (int l = 0; l < lines; l++) {
        for (int i = 0; i < len; i++) {
            uint8_t v = is_max ? 0 : 255;
            for (int k = -radius; k <= radius; k++) {
                int j = i + k;
                if (j < 0 || j >= len) continue;
                uint8_t s = horizontal ? img[(size_t)l * w + j] : img[(size_t)j * w + l];
                v = is_max ? (std::max)(v, s) : (std::min)(v, s);
            }
            if (horizontal) out[(size_t)l * w + i] = v;
            else out[(size_t)i * w + l] = v;
        }
    }
    img.swap(out);
}

inline void morph_open(std::vector<uint8_t>& img, int w, int h, int radius) {
    morph_1d(img, w, h, radius, true, false);
    morph_1d(img, w, h, radius, false, false);
    morph_1d(img, w, h, radius, true, true);
    morph_1d(img, w, h, radius, false, true);
}

// 3-4 chamfer distance transform of nonzero pixels
inline std::vector<int> chamfer_dt(const std::vector<uint8_t>& bin, int w, int h) {
    const int INF = 1 << 28;
    std::vector<int> d(bin.size());
    for (size_t i = 0; i < bin.size(); i++) d[i] = bin[i] ? INF : 0;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            int& v = d[(size_t)y * w + x];
            if (!v) continue;
            if (x > 0) v = (std::min)(v, d[(size_t)y * w + x - 1] + 3);
            if (y > 0) v = (std::min)(v, d[(size_t)(y - 1) * w + x] + 3);
            if (x > 0 && y > 0) v = (std::min)(v, d[(size_t)(y - 1) * w + x - 1] + 4);
            if (x < w - 1 && y > 0) v = (std::min)(v, d[(size_t)(y - 1) * w + x + 1] + 4);
        }
    for (int y = h - 1; y >= 0; y--)
        for (int x = w - 1; x >= 0; x--) {
            int& v = d[(size_t)y * w + x];
            if (!v) continue;
            if (x < w - 1) v = (std::min)(v, d[(size_t)y * w + x + 1] + 3);
            if (y < h - 1) v = (std::min)(v, d[(size_t)(y + 1) * w + x] + 3);
            if (x < w - 1 && y < h - 1) v = (std::min)(v, d[(size_t)(y + 1) * w + x + 1] + 4);
            if (x > 0 && y < h - 1) v = (std::min)(v, d[(size_t)(y + 1) * w + x - 1] + 4);
        }
    return d;
}

// connected components (4-neighbour) on a binary image
inline int label_components(const std::vector<uint8_t>& bin, int w, int h, std::vector<int>& labels,
                            std::vector<int>& areas) {
    labels.assign(bin.size(), 0);
    areas.assign(1, 0);
    int next = 1;
    std::vector<int> stack;
    for (size_t s = 0; s < bin.size(); s++) {
        if (!bin[s] || labels[s]) continue;
        int area = 0;
        stack.push_back((int)s);
        labels[s] = next;
        while (!stack.empty()) {
            int i = stack.back();
            stack.pop_back();
            area++;
            int x = i % w, y = i / w;
            const int nb[4] = {x > 0 ? i - 1 : -1, x < w - 1 ? i + 1 : -1,
                               y > 0 ? i - w : -1, y < h - 1 ? i + w : -1};
            for (int n : nb)
                if (n >= 0 && bin[n] && !labels[n]) { labels[n] = next; stack.push_back(n); }
        }
        areas.push_back(area);
        next++;
    }
    return next - 1;
}

// interior points of salient bright blobs (candidate prompts for small objects)
inline std::vector<std::pair<float, float>> contour_candidate_points(const Image& frame, int max_n = 16) {
    int w = frame.w, h = frame.h;
    std::vector<uint8_t> gray = to_gray(frame);
    uint8_t t = otsu_threshold(gray);
    std::vector<uint8_t> bin(gray.size());
    for (size_t i = 0; i < gray.size(); i++) bin[i] = gray[i] > t ? 255 : 0;
    morph_open(bin, w, h, 4);  // ~9x9 kernel

    std::vector<int> labels, areas;
    int n = label_components(bin, w, h, labels, areas);
    double min_area = (std::max)(800.0, 0.0007 * w * h), max_area = 0.2 * w * h;
    std::vector<int> ids;
    for (int i = 1; i <= n; i++)
        if (areas[i] > min_area && areas[i] < max_area) ids.push_back(i);
    std::sort(ids.begin(), ids.end(), [&](int a, int b) { return areas[a] > areas[b]; });
    if ((int)ids.size() > max_n) ids.resize(max_n);
    if (ids.empty()) return {};

    std::vector<int> dt = chamfer_dt(bin, w, h);
    std::vector<std::pair<float, float>> pts;
    for (int id : ids) {  // deepest interior point of each blob
        int best = -1, best_d = -1;
        for (size_t i = 0; i < labels.size(); i++)
            if (labels[i] == id && dt[i] > best_d) { best_d = dt[i]; best = (int)i; }
        if (best >= 0) pts.push_back({(float)(best % w), (float)(best / w)});
    }
    return pts;
}

// Moore-neighbour boundary tracing of the largest component of a mask
inline std::vector<std::pair<int, int>> trace_contour(const std::vector<uint8_t>& mask, int w, int h) {
    std::vector<int> labels, areas;
    int n = label_components(mask, w, h, labels, areas);
    if (!n) return {};
    int big = 1;
    for (int i = 2; i <= n; i++)
        if (areas[i] > areas[big]) big = i;
    auto inside = [&](int x, int y) {
        return x >= 0 && x < w && y >= 0 && y < h && labels[(size_t)y * w + x] == big;
    };
    int sx = -1, sy = -1;
    for (int i = 0; i < (int)labels.size() && sx < 0; i++)
        if (labels[i] == big) { sx = i % w; sy = i / w; }
    static const int DX[8] = {1, 1, 0, -1, -1, -1, 0, 1};
    static const int DY[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    std::vector<std::pair<int, int>> contour;
    int cx = sx, cy = sy, dir = 6;  // came from above
    size_t guard = (size_t)w * h * 4;
    do {
        contour.push_back({cx, cy});
        int start = (dir + 6) % 8;  // backtrack then rotate clockwise
        bool moved = false;
        for (int k = 0; k < 8; k++) {
            int d = (start + k) % 8;
            if (inside(cx + DX[d], cy + DY[d])) {
                cx += DX[d];
                cy += DY[d];
                dir = d;
                moved = true;
                break;
            }
        }
        if (!moved) break;  // isolated pixel
    } while ((cx != sx || cy != sy) && contour.size() < guard);
    return contour;
}

// Ramer-Douglas-Peucker polyline simplification
inline void rdp(const std::vector<std::pair<int, int>>& pts, size_t i0, size_t i1, float eps,
                std::vector<uint8_t>& keep) {
    if (i1 <= i0 + 1) return;
    float x0 = (float)pts[i0].first, y0 = (float)pts[i0].second;
    float x1 = (float)pts[i1].first, y1 = (float)pts[i1].second;
    float dx = x1 - x0, dy = y1 - y0;
    float len = std::sqrt(dx * dx + dy * dy);
    float dmax = -1;
    size_t imax = i0;
    for (size_t i = i0 + 1; i < i1; i++) {
        float d;
        if (len < 1e-6f) {
            float ex = pts[i].first - x0, ey = pts[i].second - y0;
            d = std::sqrt(ex * ex + ey * ey);
        } else {
            d = std::fabs(dy * pts[i].first - dx * pts[i].second + x1 * y0 - y1 * x0) / len;
        }
        if (d > dmax) { dmax = d; imax = i; }
    }
    if (dmax > eps) {
        keep[imax] = 1;
        rdp(pts, i0, imax, eps, keep);
        rdp(pts, imax, i1, eps, keep);
    }
}

inline std::vector<std::pair<float, float>> simplify_contour(
    const std::vector<std::pair<int, int>>& c, float eps = 1.5f) {
    if (c.size() < 3) {
        std::vector<std::pair<float, float>> out;
        for (auto& p : c) out.push_back({(float)p.first, (float)p.second});
        return out;
    }
    std::vector<uint8_t> keep(c.size(), 0);
    keep.front() = keep.back() = 1;
    size_t mid = c.size() / 2;  // split closed loop at two anchors
    keep[mid] = 1;
    rdp(c, 0, mid, eps, keep);
    rdp(c, mid, c.size() - 1, eps, keep);
    std::vector<std::pair<float, float>> out;
    for (size_t i = 0; i < c.size(); i++)
        if (keep[i]) out.push_back({(float)c[i].first, (float)c[i].second});
    return out;
}

// Build a full-resolution object (mask, centroid, bbox, polygon) from
// low-res decoder logits. area == 0 means "empty mask".
inline DetectedObject object_from_logits(Sam& sam, const std::vector<float>& logits, float score) {
    DetectedObject o;
    o.mask = sam.lowres_to_full(logits);
    const int w = sam.frame_w(), h = sam.frame_h();
    // keep only the largest component: a low-granularity mask can carry stray fragments
    // (a shadow band under an engraving) that would stretch the bbox and shift the centroid
    // away from the polygon, which only ever traces the largest component
    {
        std::vector<int> labels, areas;
        int n = label_components(o.mask, w, h, labels, areas);
        if (n > 1) {
            int big = 1;
            for (int i = 2; i <= n; i++)
                if (areas[i] > areas[big]) big = i;
            for (size_t i = 0; i < o.mask.size(); i++)
                if (o.mask[i] && labels[i] != big) o.mask[i] = 0;
        }
    }
    long area = 0;
    double sx = 0, sy = 0;
    int minx = w, miny = h, maxx = -1, maxy = -1;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            if (o.mask[(size_t)y * w + x]) {
                area++;
                sx += x;
                sy += y;
                minx = (std::min)(minx, x);
                maxx = (std::max)(maxx, x);
                miny = (std::min)(miny, y);
                maxy = (std::max)(maxy, y);
            }
    o.area = (int)area;
    o.score = score;
    if (area > 0) {
        o.cx = (float)(sx / area);
        o.cy = (float)(sy / area);
        o.bx = minx;
        o.by = miny;
        o.bw = maxx - minx + 1;
        o.bh = maxy - miny + 1;
        o.polygon = simplify_contour(trace_contour(o.mask, w, h));
    }
    return o;
}

// ---- the detector ----
class EverythingDetector {
public:
    int grid_x = 10, grid_y = 6, max_objects = 20;
    static constexpr double IOU_MIN = 0.88;        // SAM's default pred_iou_thresh; golden parts >= 0.94
    static constexpr double STABILITY_MIN = 0.85;  // golden bed.jpg parts score >= 0.90; lit bed patches ~0.7
    int decode_workers = 1;  // ORT intra-op parallelism already saturates x64; measure before raising
    double last_ms = 0;
    std::function<void(int, int)> on_progress;  // (decoded prompts, total); called from decode threads

    std::vector<DetectedObject> detect(Sam& sam, const Image& frame, bool reuse_encoding = false) {
        auto t0 = std::chrono::steady_clock::now();
        int w = frame.w, h = frame.h;
        std::vector<std::pair<float, float>> pts;
        for (int j = 0; j < grid_y; j++)
            for (int i = 0; i < grid_x; i++)
                pts.push_back({w * (i + 0.5f) / grid_x, h * (j + 0.5f) / grid_y});
        for (auto& p : contour_candidate_points(frame)) pts.push_back(p);
        if (!reuse_encoding || !sam.has_embedding() || sam.frame_w() != w || sam.frame_h() != h)
            sam.encode(frame);

        // image-content border band in the 256x256 padded-square mask space
        int nw256 = sam.content_w256(), nh256 = sam.content_h256();
        std::vector<uint8_t> border((size_t)256 * 256, 0);
        for (int x = 0; x < nw256; x++)
            for (int y : {0, 1, (std::max)(0, nh256 - 2), nh256 - 1}) border[(size_t)y * 256 + x] = 1;
        for (int y = 0; y < nh256; y++)
            for (int x : {0, 1, (std::max)(0, nw256 - 2), nw256 - 1}) border[(size_t)y * 256 + x] = 1;
        int border_total = 0;
        for (uint8_t b : border) border_total += b;

        // decode all prompts in parallel (Ort::Session::Run is thread-safe;
        // the decoder session runs 1 intra-op thread each, so calls scale)
        std::vector<std::vector<LowResMask>> lowres(pts.size());
        int workers = decode_workers > 0 ? decode_workers : (int)std::thread::hardware_concurrency();
        workers = (std::max)(1, (std::min)((std::min)(workers, (int)pts.size()), 16));
        std::atomic<size_t> next_idx{0};
        std::atomic<int> done{0};
        auto work = [&] {
            size_t i;
            while ((i = next_idx.fetch_add(1)) < pts.size()) {
                lowres[i] = sam.decode_point_all(pts[i].first, pts[i].second);
                int d = ++done;
                if (on_progress && (d % 10 == 0 || d == (int)pts.size())) on_progress(d, (int)pts.size());
            }
        };
        std::vector<std::thread> pool;
        for (int t = 1; t < workers; t++) pool.emplace_back(work);
        work();
        for (auto& th : pool) th.join();

        struct Cand {
            float score;
            std::vector<uint8_t> mb;  // 256x256 binary
            std::vector<float> logits;
            float cx, cy;  // low-res centroid
            long area = 0;
            float stability = 0, border = 0;
        };
        std::vector<Cand> cands;
        for (size_t pi = 0; pi < pts.size(); pi++)
        for (LowResMask& lr : lowres[pi]) {
            if (lr.iou < IOU_MIN) continue;
            Cand c;
            c.mb.resize((size_t)256 * 256);
            long area = 0, border_hits = 0, tight = 0, loose = 0;
            double sx = 0, sy = 0;
            for (int y = 0; y < 256; y++)
                for (int x = 0; x < 256; x++) {
                    size_t i = (size_t)y * 256 + x;
                    float l = lr.logits[i];
                    uint8_t on = l > 0.f ? 1 : 0;
                    c.mb[i] = on;
                    tight += l > 1.f;
                    loose += l > -1.f;
                    if (on) {
                        area++;
                        sx += x;
                        sy += y;
                        border_hits += border[i];
                    }
                }
            if (area < 150 || area > 0.25 * 256 * 256) continue;      // glints / whole-scene
            if ((double)border_hits / border_total > 0.08) continue;  // backdrop
            // stability (SAM's own filter): a crisp object barely changes when the logit
            // threshold moves +-1; a lit patch of bed with a soft edge shrinks a lot
            if (!loose || (double)tight / loose < STABILITY_MIN) continue;
            c.score = lr.iou;
            c.stability = loose ? (float)tight / loose : 0.f;
            c.border = (float)border_hits / border_total;
            c.logits = std::move(lr.logits);
            c.cx = (float)(sx / area);
            c.cy = (float)(sy / area);
            c.area = area;
            cands.push_back(std::move(c));
        }
        std::sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.score > b.score; });

        std::vector<Cand> kept;
        for (auto& c : cands) {
            bool dup = false;
            for (auto& k : kept) {
                float dx = c.cx - k.cx, dy = c.cy - k.cy;
                // same centre AND similar size = same object; a part centred on its parent is not
                double ratio = (double)(std::min)(c.area, k.area) / (std::max)(c.area, k.area);
                if (dx * dx + dy * dy < 16 && ratio > 0.5) { dup = true; break; }  // centers < 4 px (low-res)
                long inter = 0, uni = 0;
                for (size_t i = 0; i < c.mb.size(); i++) {
                    inter += c.mb[i] & k.mb[i];
                    uni += c.mb[i] | k.mb[i];
                }
                if (uni && (double)inter / uni > 0.5) { dup = true; break; }
            }
            if (!dup) {
                kept.push_back(std::move(c));
                if ((int)kept.size() >= max_objects) break;
            }
        }

        std::vector<DetectedObject> objects;
        for (auto& k : kept) {
            DetectedObject o = object_from_logits(sam, k.logits, k.score);
            if (o.area < 300) continue;
            o.stability = k.stability;
            o.border = k.border;
            objects.push_back(std::move(o));
        }
        last_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        return objects;
    }
};

}  // namespace segment
