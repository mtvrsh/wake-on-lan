#!/usr/bin/env python
import argparse
import socket
import sys
from python import wol

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", default="255.255.255.255")
    parser.add_argument("-p", default=40000)
    parser.add_argument("MAC")
    args = parser.parse_args()

    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    s.settimeout(5)
    s.bind((args.i, args.p))

    resp, ip = s.recvfrom(150)
    print(f"{len(resp)} bytes from {ip[0]}")

    if len(resp) != wol._packet_len:
        print(f"bad packet length: {len(resp)}")
        sys.exit(1)

    if resp != wol.magic_from(args.MAC):
        print("received magic packet does not match expected packet")
        print(resp)
        print([f"{b:#x}" for b in resp])
        sys.exit(1)
