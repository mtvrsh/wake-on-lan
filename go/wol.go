package main

import (
	"bytes"
	"encoding/hex"
	"flag"
	"fmt"
	"io"
	"net"
	"os"
	"regexp"
	"strings"
)

func isValid(mac string) bool {
	ok, _ := regexp.MatchString("^([0-9a-f]{2}[:-]?){5}[0-9a-f]{2}$", mac)
	return ok
}

func sendMagic(dest io.Writer, mac string) error {
	mac = strings.Map(func(r rune) rune {
		if r == ':' || r == '-' {
			return -1
		}
		return r
	}, mac)
	hexMac, err := hex.DecodeString(mac)
	if err != nil {
		return err
	}

	const pktLen = 102
	buf := make([]byte, 0, pktLen)
	magicPacket := bytes.NewBuffer(buf)

	for i := 0; i < 6; i++ {
		magicPacket.WriteByte(0xff)
	}
	for i := 6; i < pktLen; {
		n, _ := magicPacket.Write(hexMac)
		i = i + n
	}

	n, err := dest.Write(magicPacket.Bytes())
	if n != pktLen {
		fmt.Printf("error: sent incomplete packet: len=%d\n", n)
	}
	return err
}

func printErr(err error) {
	fmt.Fprintf(os.Stderr, "error: %v\n", err)
}

func main() {
	address := flag.String("i", "255.255.255.255", "broadcast address")
	port := flag.String("p", "40000", "destination port")
	flag.Parse()

	addr, err := net.ResolveUDPAddr("udp", fmt.Sprintf("%s:%s", *address, *port))
	if err != nil {
		printErr(err)
		os.Exit(1)
	}
	sock, err := net.DialUDP("udp", nil, addr)
	if err != nil {
		printErr(err)
		os.Exit(1)
	}

	for _, arg := range flag.Args() {
		if !isValid(arg) {
			fmt.Fprintf(os.Stderr, "error: invalid mac address %q\n", arg)
			continue
		}
		if err := sendMagic(sock, arg); err != nil {
			printErr(err)
		}
	}
}
