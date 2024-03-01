#!/usr/bin/env python
import re
import socket

_packet_len = 102


def send_magic(mac_address: str, addr="255.255.255.255", port=40000):
    WakeOnLan(addr, port).send_magic(mac_address)


def magic_from(mac_address: str) -> bytes:
    m = bytes.fromhex(mac_address.replace(":", "").replace("-", ""))
    return b"\xff" * 6 + m * 16


def is_valid_mac(mac: str) -> bool:
    return bool(
        re.match(
            r"[0-9a-f]{2}([:-]?)[0-9a-f]{2}(\1[0-9a-f]{2}){4}$", mac.lower()
        )
    )


class WakeOnLan:
    def __init__(self, addr: str, port: int):
        self._socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._socket.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        self._socket.settimeout(5)
        self._socket.connect((addr, port))

    def send_magic(self, mac_address: str):
        n = self._socket.send(magic_from(mac_address))
        if n != _packet_len:
            raise RuntimeError("sent only part of magic packet")


if __name__ == "__main__":
    import argparse
    import sys

    parser = argparse.ArgumentParser(
        description="Wake up devices with magic packet."
    )
    parser.add_argument(
        "-i",
        metavar="IP",
        default="255.255.255.255",
        help="broadcast address (default: %(default)s)",
    )
    parser.add_argument(
        "-p",
        metavar="PORT",
        default=40000,
        help="destination port (default: %(default)s)",
    )
    parser.add_argument("MAC", nargs="+", help="address of device to wake")
    args = parser.parse_args()

    wol = WakeOnLan(args.i, args.p)

    for m in args.MAC:
        if not is_valid_mac(m):
            print(f"invalid mac address: {m}", file=sys.stderr)
            continue
        wol.send_magic(m)
