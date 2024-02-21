#!/bin/sh -e

MAC="af:e2:d2:40:5c:16"
BRD="192.168.0.255"

after_1s() {
	sleep 1s
	"$@"
}

test_wol() {
	if ./test_server.py $MAC $2 > /dev/null; then
		echo "$1": OK
	else
		echo "$1": failed
	fi
}

if command -v wol > /dev/null; then
	after_1s wol $MAC > /dev/null &
	test_wol "wol(1)"
	after_1s wol $MAC -i $BRD > /dev/null &
	test_wol "wol(1) subnet" "-i $BRD"
fi

after_1s c/build/wol $MAC &
test_wol "c"
after_1s c/build/wol -i $BRD $MAC &
test_wol "c subnet" "-i $BRD"

after_1s go-wol/go-wol $MAC &
test_wol "go"
after_1s go-wol/go-wol -i $BRD $MAC &
test_wol "go subnet" "-i $BRD"

after_1s python/wol.py $MAC &
test_wol "python"
after_1s python/wol.py -i $BRD $MAC &
test_wol "python subnet" "-i $BRD"

after_1s rust/target/debug/wol $MAC &
test_wol "rust"
after_1s rust/target/debug/wol -i $BRD $MAC &
test_wol "rust subnet" "-i $BRD"
