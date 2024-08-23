#include <cstdint>
#include <bitset>
#include <vector>
#include <array>

using namespace std;

using Bitset32 = bitset<32>;
using ByteArray32 = array<unsigned char, 32>;
using namespace std;
tuple<vector<Bitset32>, uint32_t, uint32_t> adjustPrefixSuffixZero(const vector<Bitset32>& src_bit_array, uint32_t padding_dot_cnt);
tuple<vector<ByteArray32>, uint32_t, uint32_t> adjustPrefixSuffixZero(const vector<ByteArray32>& src_bit_array, uint32_t padding_dot_cnt);