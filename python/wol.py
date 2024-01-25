#!/usr/bin/env python
import re
import socket


def send_magic(mac_address: str, addr: tuple[str, int]):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    s.sendto(magic_from(mac_address), addr)


# for testing
def magic_from(mac_address: str) -> bytes:
    m = bytes.fromhex(mac_address.replace(":", "").replace("-", ""))
    return b"\xff" * 6 + m * 16


def is_valid_mac(mac: str) -> bool:
    return bool(
        re.match(
            r"[0-9a-f]{2}([:-]?)[0-9a-f]{2}(\1[0-9a-f]{2}){4}$", mac.lower()
        )
    )


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

    for m in args.MAC:
        if not is_valid_mac(m):
            print(f"invalid mac address: {m}", file=sys.stderr)
            continue
        send_magic(m, (args.i, args.p))
