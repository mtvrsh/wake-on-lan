#!/bin/sh
set -e

port=9
addr=255.255.255.255

usage() {
	printf "Usage: %s [OPTIONS] MAC...\n" "$0"
	printf "  -i, --ipaddr ADDR\tBroadcast address (default: %s)\n" "$addr"
	printf "  -p, --port PORT\tDestination portn (default: %s)\n" "$port"
	printf "  -h, --help\t\tPrint help\n"
	exit 2
}

make_magic() {
	echo ffff ffff ffff | xxd -r -p > "$1"
	for _ in $(seq 16); do
		echo "$2" | xxd -r -p >> "$1"
	done
}

send_magic() {
	if echo "$1" | grep -qE "^([0-9a-fA-F]{2}[:-]?){5}[0-9a-fA-F]{2}$"; then
		pkt=mktemp
		make_magic "$pkt" "$1"
		nc -b -u -q1 "$addr" "$port" < "$pkt"
		rm -f $pkt
	else
		echo "$0": invalid MAC address: \""$1"\"
		return
	fi
}

# main

if ! command -v xxd > /dev/null; then
	echo xxd not found
	exit 1
fi
if ! command -v nc > /dev/null; then
	echo "nc (netcat) not found" >&2
	exit 1
fi

[ "$1" = "" ] && usage

while true; do
	case $1 in
		"")
			exit 0
			;;
		"-i" | "--ipaddr")
			addr="$2"
			shift 2
			;;
		"-p" | "--port")
			port="$2"
			shift 2
			;;
		"-h" | "--help")
			usage
			;;
		*)
			send_magic "$1"
			shift
			;;
	esac
done
