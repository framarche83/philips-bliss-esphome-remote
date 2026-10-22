"""Listens from the PC to FanLamp V2 commands and to the echo of the Philips Bliss fan.

For every command seen it prints who sent it (10 00 56 = a remote, original or this one;
11 00 56 = the fan's echo, i.e. the command was received), the decoded command and
whether the signature is valid. Each packet is repeated many times: one line is printed
per (prefix, tx) pair. At the end it lists the remote IDs seen.

Usage (Python with bleak and pycryptodome, see requirements.txt):
  python listen_bliss.py [seconds]
"""
import asyncio
import sys
import time

from bleak import BleakScanner

from fanlamp_v2 import decode

UUID = "000008f0-0000-1000-8000-00805f9b34fb"
NAMES = {0x09: "light toggle", 0x10: "light ON", 0x11: "light OFF", 0x15: "direction", 0x21: "light levels",
         0x28: "pair", 0x31: "fan"}
WHO = {"100056": "COMMAND", "110056": "ECHO   "}


async def main(seconds: float) -> None:
    seen: set[tuple[str, int]] = set()
    unknown: set[bytes] = set()
    remotes: dict[tuple[int, int], int] = {}  # (id, index) -> packets with a valid signature
    t0 = time.time()

    def cb(dev, adv):
        data = adv.service_data.get(UUID)
        if not data:
            return
        d = decode(data)
        if not d:
            # Service data 0x08F0 that is not V2: maybe another variant of the protocol.
            if data not in unknown:
                unknown.add(data)
                print(f"{time.time() - t0:6.1f}s  NOT V2   {data.hex()}  rssi={adv.rssi}  {dev.address}")
            return
        if d["pfx"] == "100056" and d["sign_ok"]:
            k = (d["id"], d["index"])
            remotes[k] = remotes.get(k, 0) + 1
        if (d["pfx"], d["tx"]) in seen:
            return
        seen.add((d["pfx"], d["tx"]))
        print(
            f"{time.time() - t0:6.1f}s  {WHO.get(d['pfx'], d['pfx'])}  tx={d['tx']:3}  "
            f"cmd=0x{d['cmd']:02X} {NAMES.get(d['cmd'], '?'):13} param={d['param']} arg={d['arg0']},{d['arg1']},{d['arg2']}  "
            f"id=0x{d['id']:08X}/{d['index']}  sign={'ok' if d['sign_ok'] else 'WRONG'}  rssi={adv.rssi}  {dev.address}"
        )

    async with BleakScanner(detection_callback=cb):
        print(f"Listening for {seconds:.0f} s... press a button on a remote near the PC.")
        await asyncio.sleep(seconds)

    print()
    if not remotes:
        print("No V2 remote with a valid signature.")
    for (dev_id, index), n in sorted(remotes.items()):
        print(f"Remote: id 0x{dev_id:08X}  index {index}  ({n} packets)")


if __name__ == "__main__":
    asyncio.run(main(float(sys.argv[1]) if len(sys.argv) > 1 else 60))
