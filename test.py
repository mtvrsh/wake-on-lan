#!/usr/bin/env python
import argparse
import socket
import subprocess
import threading

from python import wol

BINS = [
    "c/build/wol",
    "go-wol/go-wol",
    "js/wol.js",
    "python/wol.py",
    "rust/target/debug/wol",
    "sh/wol.sh",
]

ADDRS = ["255.255.255.255", "192.168.0.255"]
PORTS = [2137, 40000]

MAC = "12:34:56:78:9A:bc"


def test(bin: str):
    eprint(f"running tests for {bin}")
    for addr in ADDRS:
        for port in PORTS:
            eprint(f"executing: {bin} -p {port} -i {addr} {MAC}")
            try:
                a = (bin, port, addr, MAC)
                server = threading.Thread(target=test_packet, args=a)
                wol_bin = threading.Thread(target=run_bin, args=a)
                server.start()
                wol_bin.start()
            except FileNotFoundError:
                eprint(f"File {bin} not found")
            wol_bin.join(5)
            server.join(5)


def test_packet(bin: str, port: int, addr: str, mac: str):
    failed = False
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    s.settimeout(2)
    s.bind((addr, port))

    try:
        resp, ip = s.recvfrom(128)
        eprint(f"{len(resp)} bytes from {ip[0]}")

        if len(resp) != wol._packet_len:
            eprint(f"bad packet length: {len(resp)}")
            failed = True

        wol_packet = wol.magic_from(mac)
        if resp != wol_packet:
            eprint("packet does not match expected")
            failed = True
            if len(resp) > len(wol_packet):
                eprint(f"too short: {len(wol_packet)}")
                return
            elif len(resp) < len(wol_packet):
                eprint(f"too long: {len(wol_packet)}")
                return
            diff_packets(wol_packet, resp)
    except TimeoutError:
        eprint("Timed out waiting for packet")
        failed = True

    if failed is True:
        print(f"{bin}: FAILED")
    else:
        print(f"{bin}: OK")


def run_bin(bin: str, port: int, addr: str, mac: str):
    try:
        subprocess.run([bin, "-p", str(port), "-i", addr, mac], check=True)
    except FileNotFoundError:
        eprint(f"File {bin} not found")


def diff_packets(packetA, packetB):
    diff1 = []
    diff2 = []
    for i, byte in enumerate(packetA):
        if byte != packetB[i]:
            diff1.append((i, packetA[i]))
            diff2.append((i, packetB[i]))

    def hexprint(diff):
        for i, b in diff:
            if verbose:
                print(f"{i}:{b:#x}", sep="", end=" ")
        eprint()

    eprint("Want:")
    hexprint(diff1)
    eprint("Got:")
    hexprint(diff2)


def eprint(s: str = ""):
    if verbose:
        print(s)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", action="store_true", help="verbose")
    parser.add_argument("binary", nargs="*", default=BINS)
    args = parser.parse_args()

    global verbose
    verbose = args.v

    for b in args.binary:
        test(b)
