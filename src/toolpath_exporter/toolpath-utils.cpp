#include "toolpath-utils.h"
#include <QtMath>

using namespace std;
/**
 * @brief Remove the suffix and prefix zeros
 *
 * @param src_bit_array
 * @return std::tuple<vector<std::bitset<32>>, uint32_t, uint32_t>
 *         - the trimmed bit array
 *         - the bit idx of the trim start
 *         - the bit idx of the trim end
 */
tuple<vector<Bitset32>, uint32_t, uint32_t> adjustPrefixSuffixZero(
      const vector<Bitset32>& src_bit_array, uint32_t padding_dot_cnt) {

  enum class SearchStage {
      kSearchForStartPos,
      kSearchForEndPos,
  };
  SearchStage search_stage = SearchStage::kSearchForStartPos;
  uint32_t trim_start_bit_idx = 0;
  uint32_t trim_end_bit_idx = 0;
  for (int i = 0; i < src_bit_array.size(); i++) {
    if (src_bit_array[i].none()) {
      continue;
    }

    if (search_stage == SearchStage::kSearchForStartPos) {
      // Search for the first black dot
      for (int j = 31; j >= 0; j--) { // search from most significant bit
        if (src_bit_array[i][j]) {
          trim_start_bit_idx = i*32 + (31-j);
          search_stage = SearchStage::kSearchForEndPos;
          break;
        }
      }
    }
    if (search_stage == SearchStage::kSearchForEndPos) {
      // Search for the last black dot
      for (int j = 0; j < 32; j++) { // search from least significant bit
        if (src_bit_array[i][j]) {
          trim_end_bit_idx = i*32 + (31-j);
          break;
        }
      }
    }

  } // end of search for loop

  // Calculate padding (with restriction of path boundary)
  trim_start_bit_idx = trim_start_bit_idx > padding_dot_cnt ? (trim_start_bit_idx - padding_dot_cnt) : 0;
  trim_end_bit_idx = qMin(trim_end_bit_idx + padding_dot_cnt,
                          uint32_t(src_bit_array.size() * 32u - 1)); // index max = size - 1

  // Trim zeros but also keep padding
  vector<std::bitset<32>> bit_array{
      src_bit_array.begin() + (trim_start_bit_idx / 32),
      src_bit_array.begin() + (trim_end_bit_idx / 32) + 1 // (+1 because the last iterator is not included)
  };
  // Align the trim first pos with the start of bit array
  int bit_shift = trim_start_bit_idx % 32;
  for (auto i = 0; i < bit_array.size(); i++) {
    if (i+1 == bit_array.size()) {
      bit_array[i] = (bit_array[i] << bit_shift);
    } else {
      bit_array[i] = (bit_array[i] << bit_shift) | (bit_array[i+1] >> (32-bit_shift)) ;
    }
  }
  // Discard all trailing zero bits (not needed, reduce data transmission)
  while (bit_array.back().none()) {
    bit_array.pop_back();
  }

  return std::make_tuple(bit_array, trim_start_bit_idx, trim_end_bit_idx);
}

std::tuple<vector<ByteArray32>, uint32_t, uint32_t> adjustPrefixSuffixZero(
    const vector<ByteArray32>& src_grayscale_array, 
    uint32_t padding_dot_cnt) {

  enum class SearchStage {
      kSearchForStartPos,
      kSearchForEndPos,
  };
  SearchStage search_stage = SearchStage::kSearchForStartPos;
  uint32_t trim_start_idx = 0;
  uint32_t trim_end_idx = 0;
  const unsigned char white_threshold = 250;  // Adjust this value as needed

  for (int i = 0; i < src_grayscale_array.size(); i++) {
    bool non_white_found = false;

    if (search_stage == SearchStage::kSearchForStartPos) {
      // Search for the first non-white dot
      for (int j = 0; j < 32; j++) {
        if (src_grayscale_array[i][j] < white_threshold) {
          trim_start_idx = i * 32 + j;
          search_stage = SearchStage::kSearchForEndPos;
          non_white_found = true;
          break;
        }
      }
    }
    
    if (search_stage == SearchStage::kSearchForEndPos) {
      // Search for the last non-white dot
      for (int j = 31; j >= 0; j--) {
        if (src_grayscale_array[i][j] < white_threshold) {
          trim_end_idx = i * 32 + j;
          non_white_found = true;
          break;
        }
      }
    }

    if (!non_white_found && search_stage == SearchStage::kSearchForEndPos) {
      break;
    }
  }

  // Calculate padding (with restriction of path boundary)
  trim_start_idx = trim_start_idx > padding_dot_cnt ? (trim_start_idx - padding_dot_cnt) : 0;
  trim_end_idx = std::min(trim_end_idx + padding_dot_cnt,
                          uint32_t(src_grayscale_array.size() * 32u - 1));

  // Trim white space but also keep padding
  vector<ByteArray32> grayscale_array{
      src_grayscale_array.begin() + (trim_start_idx / 32),
      src_grayscale_array.begin() + (trim_end_idx / 32) + 1
  };

  // Align the trim first pos with the start of grayscale array
  int shift = trim_start_idx % 32;
  if (shift > 0) {
    for (size_t i = 0; i < grayscale_array.size() - 1; i++) {
      std::rotate(grayscale_array[i].begin(), grayscale_array[i].begin() + shift, grayscale_array[i].end());
      std::copy_n(grayscale_array[i + 1].begin(), shift, grayscale_array[i].end() - shift);
    }
    if (grayscale_array.size() > 1) {
      std::rotate(grayscale_array.back().begin(), grayscale_array.back().begin() + shift, grayscale_array.back().end());
    }
  }

  // Discard all trailing white values (not needed, reduce data transmission)
  while (!grayscale_array.empty() && 
         std::all_of(grayscale_array.back().begin(), grayscale_array.back().end(), 
                     [&](unsigned char v) { return v >= white_threshold; })) {
    grayscale_array.pop_back();
  }

  return std::make_tuple(grayscale_array, trim_start_idx, trim_end_idx);
}
