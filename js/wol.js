#!/usr/bin/env node

const dgram = require("node:dgram");
const { argv } = require("node:process");

function usage() {
  console.log("Usage: wol.js [OPTIONS] MAC...");
  console.log("  -i, --ipaddr\tBroadcast address (default: 255.255.255.255)");
  console.log("  -p, --port\tDestinatin port (default: 9)");
  console.log("  -h, --help\tPrint help");
  process.exit(2);
}

function parse_args() {
  var args = {
    port: 9,
    addr: "255.255.255.255",
    macs: [],
  };

  for (var i = 2; i < argv.length; i++) {
    switch (argv[i]) {
      case "--help":
      case "-h":
        usage();
      case "--ipaddr":
      case "-i":
        if (!argv[i + 1]) {
          console.error("value is required for \"%s\"", argv[i]);
          process.exit(1);
        }
        args.addr = argv[i + 1];
        i++;
        break;
      case "--port":
      case "-p":
        if (!argv[i + 1]) {
          console.error("value is required for \"%s\"", argv[i]);
          process.exit(1);
        }
        args.port = argv[i + 1];
        i++;
        break;
      default:
        args.macs.push(argv[i]);
        break;
    }
  }

  if (args.macs.length == 0) {
    usage();
  }
  return args;
}

function main() {
  var args = parse_args();
  const socket = dgram.createSocket("udp4");
  socket.bind(0, () => {
    socket.setBroadcast(true);
  });

  for (var mac of args.macs) {
    var bmac = parse_mac(mac);
    if (!bmac) {
      console.error("\"%s\" is not valid mac address", mac);
      continue;
    }
    var magic_pkt = make_magic(bmac);
    socket.send(magic_pkt, 0, magic_pkt.length, args.port, args.addr);
  }

  socket.unref(); // ???
}

function parse_mac(mac) {
  const re = /^[0-9a-f]{2}([:-]?)[0-9a-f]{2}(\1[0-9a-f]{2}){4}$/;
  if (!re.test(mac.toLowerCase())) {
    return;
  }
  mac = mac.replaceAll("-", ":");
  var mac = mac.split(":");
  var bytes = new Uint8Array(6);
  for (var i = 0; i < 6; i++) {
    bytes[i] = parseInt(mac[i], 16);
  }
  return bytes;
}

function make_magic(bytes) {
  const pkt = new Uint8Array(102);
  pkt.fill(255, 0, 6);
  for (var i = 6; i < 102; i += 6) {
    pkt.set(bytes, i);
  }
  return pkt;
}

if (require.main === module) {
  main();
}
