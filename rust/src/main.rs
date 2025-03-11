use std::{net::UdpSocket, process::ExitCode, time::Duration};

use argh::FromArgs;
use macaddr::MacAddr6;

/// Cross platform wake on lan client.
#[derive(FromArgs, Debug)]
#[argh(help_triggers("-h", "--help"))]
struct Cli {
    /// broadcast address
    #[argh(option, short = 'i', default = "String::from(\"255.255.255.255\")")]
    addr: String,

    /// destination port
    #[argh(option, short = 'p', default = "40000")]
    port: u16,

    /// MAC addresses
    #[argh(positional, greedy)]
    mac: Vec<String>,
}

fn main() -> ExitCode {
    let args: Cli = argh::from_env();

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
    if let Err(e) = socket.connect(format!("{}:{}", args.addr, args.port)) {
        eprintln!(
            "error: failed to connect socket to \"{}:{}\": {e}",
            args.addr, args.port
        );
        return ExitCode::FAILURE;
    }

    if args.mac.is_empty() {
        let cmd: Vec<String> = std::env::args().collect();
        let cmd = std::path::Path::new(&cmd[0])
            .file_name()
            .and_then(|s| s.to_str())
            .unwrap_or(cmd[0].as_str());
        eprintln!("error: too few arguments, try `{cmd} --help`");
        return ExitCode::FAILURE;
    }

    for mac in args.mac {
        let m = match mac.parse::<MacAddr6>() {
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
