"""Compares bliss::encode() (C++) with tools/fanlamp_v2.py and checks that decode() reads it back.

Usage: python test_bliss.py <file with the output of test_bliss>
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "tools"))
from fanlamp_v2 import decode, encode  # noqa: E402

ok = True
for line in open(sys.argv[1], encoding="utf-8").read().split("\n"):
    if not line.strip():
        continue
    *nums, hexout = line.split()
    cmd, a0, a1, a2, param, tx, seed, dev_id, index = map(int, nums)
    data = encode(cmd, dev_id, index, tx, seed, (0x10, 0x00, 0x56), 0x0400, a0, a1, a2, param)
    expected = bytes([0x02, 0x01, 0x02, len(data) + 3, 0x16, 0xF0, 0x08]) + data
    got = bytes.fromhex(hexout)
    dec = decode(got[7:])
    good = (got == expected and dec is not None and dec["sign_ok"] and dec["cmd"] == cmd and dec["tx"] == tx
            and dec["id"] == dev_id and dec["index"] == index and dec["param"] == param)
    ok &= good
    print(f"{'OK ' if good else 'ERR'} cmd=0x{cmd:02X} tx={tx:3} seed=0x{seed:04X} id=0x{dev_id:08X}/{index:<2}  {got.hex()}")
    if got != expected:
        print(f"    expected {expected.hex()}")
print("ALL EQUAL" if ok else "DIFFERENCES FOUND")
sys.exit(0 if ok else 1)
