# Philips Bliss ESPHome remote

ESPHome firmware that turns an **ESP32-2424S012C** (Sunton ESP32-C3 board with a round 1.28" touch display, about €25) into a remote for the **Philips Bliss ceiling fan with light**. The original remote talks to the fan over BLE advertising, using the FanLamp Pro V2 protocol. This board sends the same packets, so:

- it works on its own, as a wall or table remote, without Home Assistant;
- with Home Assistant it exposes the fan and the light as normal entities.

> Not affiliated with, endorsed by or supported by Philips or Signify. "Philips" and "Bliss" are used only to say which product this works with. Use at your own risk.

## What it does

| Function | On the display | In Home Assistant |
|---|---|---|
| Fan on/off (restarts at the last speed) | centre button | `fan` entity |
| Fan speed 1–6 | − / + buttons, ring shows the speed | speed slider |
| Fan direction | — | direction |
| Light on/off | bulb button | `light` entity |
| Realign the assumed light state | long press on the bulb | **Realign light** button |
| Pair with a fan | — | **Pair with Bliss** button |

The screen turns off after 30 s. The first touch only wakes it up.

### Things worth knowing about the Bliss

These are observations on our unit, made by sniffing the original remote and testing:

- **The light only has a toggle** (command `0x09`). The fan never reports its state; its echo only confirms that a command arrived. The remote therefore *assumes* the light state and remembers it across reboots. If the light is changed with the original remote, the display can end up inverted: a long press on the bulb (or **Realign light**) flips the icon without sending anything.
- **Quick off/on changes the colour temperature.** If the light is switched back on a few seconds after being switched off, the Bliss steps to the next tone: it did at 2 s, not at 8 s. The remote therefore holds a quick re-on back until 8 s have passed since the off (`light_min_off_ms`), so the tone stays the same. Waiting longer than that switches it on immediately.
- **The levels command `0x21` had no visible effect** (the fan echoes it). That is why brightness and colour temperature are off by default (`light_adjustable: "false"`).
- **Fan:** command `0x31`, `arg0 = 0x20` (6-speed mode), `arg1 = speed 0–6`, where 0 = off.
- **The fan filters by remote ID, but don't copy the original remote's ID.** In our house all the Bliss units answered to the ID of one original remote, so copying it drove every fan at once. Each board here uses its own ID (derived from its MAC address by default) and is paired with one fan.

## Hardware

**ESP32-2424S012C** by Sunton, the round 1.28" board with capacitive touch, sold with a case for about €25. The "C" is the touch version; get that one, ideally with the case. Search for "ESP32-2424S012C".

- ESP32-C3, 4 MB flash, no PSRAM
- GC9A01A 240×240 round display
- CST816S capacitive touch
- USB-C for power and the first flash

| Signal | GPIO |
|---|---|
| SPI CLK / MOSI | 6 / 7 |
| Display CS / DC | 10 / 2 (no reset line) |
| Backlight | 3 |
| Touch I2C SDA / SCL | 4 / 5 |
| Touch INT / RST | 0 / 1 (address 0x15) |

## Build and flash

Requires ESPHome 2026.8.2 or newer.

1. Copy `secrets.example.yaml` to `secrets.yaml` and fill it in. The API key must be your own (`openssl rand -base64 32`): ESPHome rejects the placeholder.
2. Optionally change `name` / `friendly_name` in `bliss-remote.yaml`. With more than one remote, give each its own name.
3. Flash the first time over USB, then over the air:
   ```
   esphome run bliss-remote.yaml
   ```

`bliss-remote-offline.yaml` is the same remote without WiFi, API and OTA, for a standalone remote that you update over USB. It still needs a `secrets.yaml`, even with dummy values.

## Pairing

A Bliss only accepts pairing during the first few seconds after it is powered on.

1. Make sure the other fans are **not** powered on during the procedure. Fans on separate wall switches that stay on are fine; they ignore the pairing.
2. Start pressing **Pair with Bliss** (Home Assistant, or the ESPHome API) every few seconds.
3. Meanwhile, switch the fan off at the wall, wait 5 s and switch it back on.
4. Tap the bulb on the display: the light should toggle.

The pairing packet is the one the original remote sends on a long press (`0x28`, `param 0x70`, about 2 s). The fan does not echo it, so the only check is that the light then responds. In our test the original remote kept working after pairing.

If you set `remote_id` yourself instead of leaving `"0"`, use a random 32-bit value that is different for every remote.

## Home Assistant

The device is discovered by the **ESPHome** integration. Add it and enter the API encryption key from your `secrets.yaml`. You get:

- `fan.*_fan` with 6 speeds and direction;
- `light.*_light`: on/off only by default. Brightness and colour temperature have no effect unless `light_adjustable` is `"true"`;
- `button.*_realign_light` and `button.*_pair_with_bliss` (configuration);
- `light.*_backlight` (configuration, display brightness).

Every light command, from the display or from Home Assistant, goes through the same logic, including the 8-second hold-back.

To try other commands, call the `send_command` action from **Developer tools → Actions**:

```yaml
action: esphome.bliss_remote_send_command
data:
  cmd: 33     # 0x21 light levels
  arg0: 0
  arg1: 255   # cold
  arg2: 0     # warm
```

## Tools

- `tools/listen_bliss.py`: listens from a PC (Windows or Linux, via [bleak](https://github.com/hbldh/bleak)) and prints every V2 command and every echo from the fan. Useful for checking what a remote or this board sends, and whether the fan received it (`ECHO` lines).
  ```
  pip install -r tools/requirements.txt
  python tools/listen_bliss.py 60
  ```
- `preview/preview.yaml`: the same screen in a 240×240 SDL window on Linux (`esphome run preview/preview.yaml`, needs `libsdl2-dev`). Fan and light are fake and only log.
- `tests/`: checks byte for byte that the C++ encoder matches the Python codec:
  ```
  cd tests && g++ -std=c++17 -I.. test_bliss.cpp -o test_bliss && ./test_bliss > out.txt && python test_bliss.py out.txt
  ```

## Protocol summary

- Advertising payload: flags `02 01 02`, then 16-bit service data UUID `0x08F0` with 24 bytes.
- Payload: prefix `10 00 56` for a command, `11 00 56` for the fan's echo; tx counter; device type `0x0400`; remote ID (4 bytes) and index; command, param, arg0–arg2.
- AES-ECB signature, XBOXES whitening and CRC-16 (CCITT), as in ha-ble-adv's `fanlamp_pro_v2` codec.
- Commands seen from the original remote: `0x09` light toggle, `0x31` fan speed/off, `0x28` pair (long press).
- Each press lasts about 0.4 s on the original remote. This firmware advertises each command for 1.5 s, one at a time.

## Credits

- [NicoIIT/ha-ble-adv](https://github.com/NicoIIT/ha-ble-adv): the FanLamp codecs this is derived from (MIT, see `THIRD_PARTY_NOTICES.md`). If you only want Home Assistant control, with no display, look at that project and its ESPHome BLE proxy.
- [ESPHome](https://esphome.io) and LVGL for the firmware and the UI.
- [Material Design Icons](https://pictogrammers.com/library/mdi/): the font is downloaded at build time.

## License

MIT, see `LICENSE`.
