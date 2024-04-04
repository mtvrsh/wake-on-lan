package main

import (
	"flag"
	"fmt"
	"io"
	"log"
	"net"
	"os"
	"path/filepath"
	"time"
)

const packetLen = 102

func main() {
	log.SetFlags(log.Lshortfile)
	flag.Usage = func() {
		fmt.Fprintf(os.Stderr, "Usage: %s [OPTIONS] MAC...\n", filepath.Base(os.Args[0]))
		flag.PrintDefaults()
	}
	address := flag.String("i", "255.255.255.255", "broadcast `addr`ess")
	port := flag.String("p", "9", "destination port `num`ber")
	flag.Parse()

	if flag.NArg() == 0 {
		flag.Usage()
		os.Exit(2)
	}

	addr := net.JoinHostPort(*address, *port)
	conn, err := net.DialTimeout("udp", addr, time.Second*5)
	if err != nil {
		log.Fatal(err)
	}

	for _, arg := range flag.Args() {
		mac, err := net.ParseMAC(arg)
		if err != nil {
			log.Print(err)
			continue
		}
		n, err := sendMagic(conn, mac)
		if err != nil {
			log.Fatal(err)
		}
		if n != packetLen {
			log.Fatalf("sent %v of %v bytes", n, packetLen)
		}
	}

	if err := conn.Close(); err != nil {
		log.Print(err)
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
