use clap::Parser;
use macaddr::MacAddr6;
use std::{net::UdpSocket, process::ExitCode, time::Duration};

#[derive(Parser)]
#[command(about, version)]
struct Cli {
    /// Broadcast address
    #[arg(short, long, value_name = "ADDR", default_value = "255.255.255.255")]
    ipaddr: String,

    /// Destination port number
    #[arg(short, long, value_name = "NUM", default_value = "9")]
    port: u16,

    /// MAC address
    #[arg(required = true)]
    mac: Vec<String>,
}

fn main() -> ExitCode {
    let args = Cli::parse();

    let socket = match UdpSocket::bind("0.0.0.0:0") {
        Ok(s) => s,
        Err(e) => {
            eprintln!("error: failed to bind socket to \"0.0.0.0\": {e}");
            return ExitCode::FAILURE;
        }
    };
    socket.set_write_timeout(Some(Duration::from_secs(5))).ok();
    if let Err(e) = socket.set_broadcast(true) {
        eprintln!("error: failed to set SO_BROADCAST: {e}");
        return ExitCode::FAILURE;
    }
    if let Err(e) = socket.connect(format!("{}:{}", args.ipaddr, args.port)) {
        eprintln!(
            "error: failed to connect socket to \"{}:{}\": {e}",
            args.ipaddr, args.port
        );
        return ExitCode::FAILURE;
    }

    for mac in args.mac {
        let m: MacAddr6 = match mac.parse() {
            Ok(m) => m,
            Err(e) => {
                eprintln!("error: \"{mac}\" is not valid MAC address: {e}");
                continue;
            }
        };
        match send_magic(&socket, m.into_array()) {
            Ok(n) if n != 102 => {
                eprintln!("error: sent {n} of 102 bytes");
                return ExitCode::FAILURE;
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
