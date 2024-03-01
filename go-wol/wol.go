package main

import (
	"flag"
	"fmt"
	"io"
	"net"
	"os"
	"time"
)

const packetLen = 102

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

	addr := net.JoinHostPort(*address, *port)
	conn, err := net.DialTimeout("udp", addr, time.Second*5)
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
		n, err := sendMagic(conn, mac)
		if err != nil {
			printErr(err)
			os.Exit(1)
		}
		if n != packetLen {
			printErr(fmt.Errorf("sent %v of %v bytes", n, packetLen))
			os.Exit(1)
		}
	}

	if err := conn.Close(); err != nil {
		printErr(err)
	}
}

// sendMagic expects valid MAC address, see net.ParseMAC.
func sendMagic(dest io.Writer, mac net.HardwareAddr) (int, error) {
	if len(mac) != 6 {
		return 0, fmt.Errorf("unsupported MAC address format")
	}
	packet := make([]byte, 6, packetLen)
	for i := 0; i < 6; i++ {
		packet[i] = 0xff
	}
	for i := 0; i < 16; i++ {
		packet = append(packet, mac...)
	}
	return dest.Write(packet)
}

func printErr(err error) {
	fmt.Fprintf(os.Stderr, "error: %v\n", err)
}
