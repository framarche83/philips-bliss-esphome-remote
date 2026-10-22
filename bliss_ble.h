#pragma once
// =============================================================================
// Philips Bliss (ceiling fan with light) — BLE advertising codec "FanLamp Pro V2"
//
// C++ port of the fanlamp_pro_v2 / remote codec from NicoIIT/ha-ble-adv
// (codecs/fanlamp.py, MIT License). This is the format the fan ACCEPTS:
//   flags 02 01 02 + service data UUID 0x08F0, prefix 10 00 56, device type 0x0400,
//   remote ID (configurable), AES signature, XBOXES whitening, CCITT CRC.
// encode() is checked byte for byte against the Python codec by tests/test_bliss.cpp.
//
// AES-128 is implemented here (single-block encryption only) instead of using mbedtls,
// so the very same code runs on the PC for the test and on the ESP32.
// =============================================================================
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace bliss {

// ID and index of this remote: a fan only obeys the remotes it is paired with.
// The YAML sets them at boot (substitutions remote_id / remote_index).
inline uint32_t device_id = 0;
inline uint8_t device_index = 0;
static const uint16_t DEVICE_TYPE = 0x0400;
static const uint8_t PREFIX[3] = {0x10, 0x00, 0x56};  // the fan's echo uses 11 00 56

// Commands. Verified on the Bliss: LIGHT_TOGGLE, FAN and PAIR.
// The others come from ha-ble-adv: the fan echoes them, but their effect is unconfirmed.
static const uint8_t CMD_LIGHT_TOGGLE = 0x09;
static const uint8_t CMD_LIGHT_ON = 0x10;
static const uint8_t CMD_LIGHT_OFF = 0x11;
static const uint8_t CMD_DIRECTION = 0x15;    // arg0: 0 forward, 1 reverse
static const uint8_t CMD_LIGHT_LEVEL = 0x21;  // arg1 cold 0-255, arg2 warm 0-255
static const uint8_t CMD_PAIR = 0x28;         // param 0x70, as sent by the original remote
static const uint8_t CMD_FAN = 0x31;          // arg0 0x20 (6 speeds), arg1 speed 0-6

static const uint8_t SBOX[256] = {
    0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
    0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
    0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
    0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
    0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16,
};

// Whitening table from fanlamp.py: 8 rows of the AES S-box.
static const uint8_t XBOXES[128] = {
    0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
    0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
    0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
    0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
    0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
    0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
    0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
    0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16,
};

static inline uint8_t xtime(uint8_t x) { return (uint8_t) ((x << 1) ^ ((x & 0x80) ? 0x1B : 0x00)); }

// AES-128 ECB, single-block encryption.
inline void aes128_encrypt(const uint8_t key[16], const uint8_t in[16], uint8_t out[16]) {
  uint8_t rk[176];
  memcpy(rk, key, 16);
  uint8_t rcon = 0x01;
  for (int i = 16; i < 176; i += 4) {
    uint8_t t[4] = {rk[i - 4], rk[i - 3], rk[i - 2], rk[i - 1]};
    if (i % 16 == 0) {
      uint8_t first = t[0];
      t[0] = SBOX[t[1]] ^ rcon;
      t[1] = SBOX[t[2]];
      t[2] = SBOX[t[3]];
      t[3] = SBOX[first];
      rcon = xtime(rcon);
    }
    for (int j = 0; j < 4; j++)
      rk[i + j] = rk[i + j - 16] ^ t[j];
  }
  uint8_t s[16];
  for (int i = 0; i < 16; i++)
    s[i] = in[i] ^ rk[i];
  for (int round = 1; round <= 10; round++) {
    uint8_t t[16];
    // SubBytes + ShiftRows (state stored by columns: byte r + 4c)
    for (int c = 0; c < 4; c++)
      for (int r = 0; r < 4; r++)
        t[r + 4 * c] = SBOX[s[r + 4 * ((c + r) & 3)]];
    if (round != 10) {  // MixColumns
      for (int c = 0; c < 4; c++) {
        uint8_t *col = t + 4 * c;
        uint8_t a0 = col[0], a1 = col[1], a2 = col[2], a3 = col[3];
        uint8_t all = a0 ^ a1 ^ a2 ^ a3;
        col[0] = a0 ^ all ^ xtime(a0 ^ a1);
        col[1] = a1 ^ all ^ xtime(a1 ^ a2);
        col[2] = a2 ^ all ^ xtime(a2 ^ a3);
        col[3] = a3 ^ all ^ xtime(a3 ^ a0);
      }
    }
    for (int i = 0; i < 16; i++)
      s[i] = t[i] ^ rk[16 * round + i];
  }
  memcpy(out, s, 16);
}

// binascii.crc_hqx: CRC-CCITT, polynomial 0x1021, no reflection.
inline uint16_t crc_hqx(const uint8_t *data, size_t len, uint16_t crc) {
  for (size_t i = 0; i < len; i++) {
    crc ^= (uint16_t) data[i] << 8;
    for (int b = 0; b < 8; b++)
      crc = (crc & 0x8000) ? (uint16_t) ((crc << 1) ^ 0x1021) : (uint16_t) (crc << 1);
  }
  return crc;
}

// Fills out[31] with the whole advertising payload: flags + service data F0 08.
inline void encode(uint8_t out[31], uint8_t cmd, uint8_t arg0, uint8_t arg1, uint8_t arg2, uint8_t param,
                   uint8_t tx, uint16_t seed) {
  uint8_t d[24] = {PREFIX[0],
                   PREFIX[1],
                   PREFIX[2],
                   tx,
                   (uint8_t) (DEVICE_TYPE & 0xFF),
                   (uint8_t) (DEVICE_TYPE >> 8),
                   (uint8_t) (device_id & 0xFF),
                   (uint8_t) ((device_id >> 8) & 0xFF),
                   (uint8_t) ((device_id >> 16) & 0xFF),
                   (uint8_t) (device_id >> 24),
                   device_index,
                   cmd,
                   0x00,
                   param,
                   arg0,
                   arg1,
                   arg2};
  // Signature: AES of bytes 1..16, key = seed, tx and a fixed tail.
  const uint8_t key[16] = {(uint8_t) (seed & 0xFF), (uint8_t) (seed >> 8), tx, 0x0D, 0xBF, 0xE6, 0x42, 0x68,
                           0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16};
  uint8_t enc[16];
  aes128_encrypt(key, d + 1, enc);
  uint16_t sign = (uint16_t) (enc[0] | (enc[1] << 8));
  if (sign == 0)
    sign = 0xFFFF;
  d[17] = sign & 0xFF;
  d[18] = sign >> 8;
  d[19] = 0x00;
  d[20] = seed & 0xFF;
  d[21] = seed >> 8;
  // Whitening of bytes 2..19 (the trailing seed stays in clear).
  const uint8_t s8 = seed & 0xFF;
  const uint8_t salt = (uint8_t) ((d[1] & 0x03) << 5);
  for (int i = 0; i < 18; i++)
    d[2 + i] ^= XBOXES[((s8 + i + 9) & 0x1F) + salt] ^ s8;
  const uint16_t crc = crc_hqx(d, 22, seed ^ 0xFFFF);
  d[22] = crc & 0xFF;
  d[23] = crc >> 8;
  static const uint8_t HEAD[7] = {0x02, 0x01, 0x02, 0x1B, 0x16, 0xF0, 0x08};
  memcpy(out, HEAD, 7);
  memcpy(out + 7, d, 24);
}

}  // namespace bliss

// -----------------------------------------------------------------------------
// Transmission (ESP32 only, inside ESPHome)
// -----------------------------------------------------------------------------
#ifdef USE_ESP32
#include <esp_gap_ble_api.h>
#include "esphome/components/esp32_ble/ble.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace bliss {

// The receiver does not require an increasing counter (tested: it accepts tx=201 after 188,
// and tx=1 after a reboot), so there is no need to persist it across reboots.
static uint8_t tx_count = 0;

// Remote ID derived from the chip's MAC address: unique per board, stable across reboots.
inline uint32_t id_from_mac() {
  uint8_t mac[6];
  esphome::get_mac_address_raw(mac);
  return ((uint32_t) mac[2] << 24) | ((uint32_t) mac[3] << 16) | ((uint32_t) mac[4] << 8) | mac[5];
}

// Starts advertising a command. stop(), called by the bliss_send script, ends it.
inline bool start(uint8_t cmd, uint8_t arg0 = 0, uint8_t arg1 = 0, uint8_t arg2 = 0, uint8_t param = 0) {
  auto *ble = esphome::esp32_ble::global_ble;
  if (ble == nullptr || !ble->is_active()) {
    ESP_LOGW("bliss", "BLE not active yet: command 0x%02X dropped", cmd);
    return false;
  }
  tx_count = (uint8_t) ((tx_count + 1) % 125);  // same as ha-ble-adv
  const uint16_t seed = (uint16_t) (1 + esphome::random_uint32() % 0xFFF5);
  uint8_t adv[31];
  encode(adv, cmd, arg0, arg1, arg2, param, tx_count, seed);

  esp_ble_adv_params_t params = {};
  params.adv_int_min = 0x20;  // 20 ms
  params.adv_int_max = 0x20;
  params.adv_type = ADV_TYPE_NONCONN_IND;
  params.own_addr_type = BLE_ADDR_TYPE_PUBLIC;
  params.channel_map = ADV_CHNL_ALL;
  params.adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY;

  esp_err_t err = esp_ble_gap_config_adv_data_raw(adv, sizeof(adv));
  if (err == ESP_OK)
    err = esp_ble_gap_start_advertising(&params);
  if (err != ESP_OK) {
    ESP_LOGW("bliss", "advertising failed (%s) for command 0x%02X", esp_err_to_name(err), cmd);
    return false;
  }
  ESP_LOGI("bliss", "cmd 0x%02X arg %u %u %u  tx %u  id 0x%08X/%u", cmd, arg0, arg1, arg2, tx_count,
           (unsigned) device_id, device_index);
  return true;
}

inline void stop() { esp_ble_gap_stop_advertising(); }

}  // namespace bliss
#endif
