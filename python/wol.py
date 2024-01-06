#!/usr/bin/env python
import re
import socket


def send_magic(mac_address: bytes, addr: tuple[str, int]):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

    magic = b"\xff" * 6 + mac_address * 16
    s.sendto(magic, (addr[0], addr[1]))


def is_valid_mac(mac: str):
    # simpler
    # return bool(re.match(r"([0-9a-f]{2}[:-]?){5}[0-9a-f]{2}$", mac.lower()))

    # more consistent match
    return bool(
        re.match(r"[0-9a-f]{2}([:-]?)[0-9a-f]{2}(\1[0-9a-f]{2}){4}$", mac.lower())
    )


if __name__ == "__main__":
    import argparse
    import sys

    parser = argparse.ArgumentParser()
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
            print(f"invalid mac address: {m}")
            sys.exit(1)
        send_magic(bytes.fromhex(m.replace(":", "").replace("-", "")), (args.i, args.p))

    # macs = [m for m in args.MAC if is_valid_mac(m)]
    # print(macs)
