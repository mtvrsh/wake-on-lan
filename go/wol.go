package main

import (
	"bytes"
	"flag"
	"fmt"
	"io"
	"net"
	"os"
)

// sendMagic expects valid MAC address, see net.ParseMAC
func sendMagic(dest io.Writer, mac net.HardwareAddr) error {
	const pktLen = 102
	buf := make([]byte, 0, pktLen)
	magicPacket := bytes.NewBuffer(buf)
	for i := 0; i < 6; i++ {
		magicPacket.WriteByte(0xff)
	}
	for i := 6; i < pktLen; {
		magicPacket.Write(mac)
		i = i + 6
	}
	_, err := dest.Write(magicPacket.Bytes())
	return err
}

func printErr(err error) {
	fmt.Fprintf(os.Stderr, "error: %v\n", err)
}

func main() {
	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage: %s [-i IP] [-p PORT] MAC-ADDRESS...\n", os.Args[0])
		flag.PrintDefaults()
	}
	address := flag.String("i", "255.255.255.255", "broadcast `address`")
	port := flag.String("p", "40000", "destination `port`")
	flag.Parse()

	if flag.NArg() == 0 {
		flag.Usage()
		os.Exit(2)
	}

	addr, err := net.ResolveUDPAddr("udp", fmt.Sprintf("%s:%s", *address, *port))
	if err != nil {
		printErr(err)
		os.Exit(1)
	}
	conn, err := net.DialUDP("udp", nil, addr)
	if err != nil {
		printErr(err)
		os.Exit(1)
	}

	for _, arg := range flag.Args() {
		mac, err := net.ParseMAC(arg)
		if err != nil {
			printErr(err)
			continue
		}
		if err := sendMagic(conn, mac); err != nil {
			printErr(err)
			os.Exit(1)
		}
	}

	if err := conn.Close(); err != nil {
		printErr(err)
	}
}
