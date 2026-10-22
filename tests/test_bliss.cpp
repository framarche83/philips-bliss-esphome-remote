// Prints the packets produced by bliss::encode() for a fixed list of cases.
// test_bliss.py compares them with tools/fanlamp_v2.py (the format the fan accepts).
//   g++ -std=c++17 -I.. test_bliss.cpp -o test_bliss && ./test_bliss > out.txt
#include <cstdio>
#include "bliss_ble.h"

int main() {
  struct Case {
    uint8_t cmd, a0, a1, a2, param, tx;
    uint16_t seed;
    uint32_t id;
    uint8_t index;
  };
  const Case cases[] = {
      {0x09, 0x00, 0x00, 0x00, 0, 1, 1, 0x1A2B3C4D, 0},
      {0x31, 0x20, 3, 0, 0, 77, 0x1234, 0x1A2B3C4D, 0},
      {0x31, 0x20, 0, 0, 0, 124, 0xFFF5, 0x1A2B3C4D, 0},
      {0x10, 0, 0, 0, 0, 200, 0x00FF, 0x1A2B3C4D, 0},
      {0x11, 0, 0, 0, 0, 0, 0xABCD, 0x1A2B3C4D, 0},
      {0x21, 0x00, 255, 128, 0, 42, 0x8001, 0x1A2B3C4D, 0},
      {0x15, 1, 0, 0, 0, 9, 0x0100, 0x1A2B3C4D, 0},
      {0x21, 0x00, 13, 0, 7, 255, 0x7FFF, 0x1A2B3C4D, 0},
      {0x28, 0, 0, 0, 0x70, 3, 0x5A5A, 0x1A2B3C4D, 0},
      // Other remotes: different ID and index
      {0x31, 0x20, 4, 0, 0, 5, 0x2222, 0x12345678, 0},
      {0x09, 0, 0, 0, 0, 33, 0x9ABC, 0xA1B2C3D4, 2},
      {0x21, 0x00, 0, 255, 0, 111, 0x0F0F, 0x00000001, 15},
  };
  for (const Case &c : cases) {
    bliss::device_id = c.id;
    bliss::device_index = c.index;
    uint8_t out[31];
    bliss::encode(out, c.cmd, c.a0, c.a1, c.a2, c.param, c.tx, c.seed);
    printf("%u %u %u %u %u %u %u %u %u ", c.cmd, c.a0, c.a1, c.a2, c.param, c.tx, c.seed, (unsigned) c.id,
           c.index);
    for (uint8_t b : out)
      printf("%02x", b);
    printf("\n");
  }
  return 0;
}
