use std::{net::UdpSocket, process::ExitCode, time::Duration};

use clap::{arg, command, value_parser};
use mac_address::MacAddress;

fn main() -> ExitCode {
    let args = command!()
        .arg(
            arg!(-i --ipaddr <ADDRESS> "broadcast address")
                .value_parser(value_parser!(String))
                .default_value("255.255.255.255"),
        )
        .arg(
            arg!(-p --port <NUM> "destination port")
                .value_parser(value_parser!(u16))
                .default_value("40000"),
        )
        .arg(arg!(<MAC> ... "MAC address"))
        .get_matches();

    let port = args
        .get_one::<u16>("port")
        .expect("port should have some value");
    let addr = args
        .get_one::<String>("ipaddr")
        .expect("ipaddr should have some value");

    let socket = match UdpSocket::bind("0.0.0.0:0") {
        Ok(s) => s,
        Err(e) => {
            eprintln!("error: failed to bind socket to \"0.0.0.0\": {e}");
            return ExitCode::FAILURE;
        }
    };
    socket.set_write_timeout(Some(Duration::new(5, 0))).ok();
    if let Err(e) = socket.set_broadcast(true) {
        eprintln!("error: failed to set SO_BROADCAST: {e}");
        return ExitCode::FAILURE;
    }
    if let Err(e) = socket.connect(format!("{addr}:{port}")) {
        eprintln!("error: failed to connect socket to \"{addr}:{port}\": {e}");
        return ExitCode::FAILURE;
    }

    let macs: Vec<&String> = args
        .get_many("MAC")
        .expect("MAC address argument not found")
        .collect();

    for mac in macs {
        let m = match mac.parse::<MacAddress>() {
            Ok(m) => m,
            Err(e) => {
                eprintln!("error: \"{mac}\" is not valid MAC address: {e}");
                continue;
            }
        };
        match send_magic(&socket, m.bytes()) {
            Ok(n) if n != 102 => {
                eprintln!("error: sent {n} bytes, should be 102");
            }
            Ok(_) => (),
            Err(e) => {
                eprintln!("error: send failed: {e}");
                return ExitCode::FAILURE;
            }
        }
    }
    ExitCode::SUCCESS
}

fn send_magic(socket: &UdpSocket, mac: [u8; 6]) -> Result<usize, std::io::Error> {
    let mut packet = vec![0xff; 6];
    for _ in 0..16 {
        packet.extend_from_slice(&mac);
    }
    socket.send(&packet)
}
