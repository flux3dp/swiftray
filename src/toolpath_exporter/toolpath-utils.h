#pragma once

#include <cstdint>
#include <bitset>
#include <vector>
#include <array>
#include <opencv2/core.hpp>
#include <QPolygon>
#include <QPolygonF>
#include <QImage>

using namespace std;

constexpr int WHITE_PIXEL = 255; // should ignored
constexpr int BLACK_PIXEL = 0; // should emit

using Bitset32 = bitset<32>;
using ByteArray32 = array<unsigned char, 32>;
using namespace std;
tuple<vector<Bitset32>, uint32_t, uint32_t> adjustPrefixSuffixZero(const vector<Bitset32>& src_bit_array, uint32_t padding_dot_cnt);
tuple<vector<ByteArray32>, uint32_t, uint32_t> adjustPrefixSuffixZero(const vector<ByteArray32>& src_bit_array, uint32_t padding_dot_cnt);

QImage imageBinarize(QImage* src, int threshold);
QImage imageTranspose(QImage* img);
bool findMinMaxPixel(QImage* img, int* min_pixel, int* max_pixel);

cv::Mat QPolygonToMat(const QPolygonF& poly);
QPolygon MatIToQPolygon(const cv::Mat& mat);
QPolygonF MatFToQPolygon(const cv::Mat& mat);
