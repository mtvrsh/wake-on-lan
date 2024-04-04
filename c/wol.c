#include <arpa/inet.h>
#include <err.h>
#include <getopt.h>
#include <libgen.h>
#include <netinet/in.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAGIC_PACKET_LEN 102

regex_t VALID_MAC_RE;

void usage(char *prog) {
  fprintf(stderr,
          "Usage: %s [OPTIONS] MAC...\n"
          "  -i ADDR    broadcast address (default: 255.255.255.255)\n"
          "  -p NUM     destination port number (default: 9)\n",
          basename(prog));
  exit(2);
}

char *get_regerror(int errcode, regex_t *compiled) {
  size_t length = regerror(errcode, compiled, NULL, 0);
  char *buffer = malloc(length);
  (void)regerror(errcode, compiled, buffer, length);
  return buffer;
}

int is_valid_mac(char *mac) { return regexec(&VALID_MAC_RE, mac, 0, NULL, 0); }

// assumes mac is valid, see is_valid_mac()
unsigned char *hex_mac_to_bytes(char *mac) {
  unsigned char *bytes = malloc(6 * sizeof(char));
  char tmp[3]; // we need only 2 but strtol expects null terminated char*
  tmp[2] = '\0';

  for (int i = 0, bindex = 0; bindex < 6;) {
    if (mac[i] == ':' || mac[i] == '-') {
      i++;
    } else {
      tmp[0] = mac[i];
      tmp[1] = mac[i + 1];
      bytes[bindex] = strtol(tmp, NULL, 16);
      bindex++;
      i = i + 2;
    }
  }
  return bytes;
}

int send_magic_packet(int sock, char *mac_hex) {
  static unsigned char magic_packet[MAGIC_PACKET_LEN];
  memset(&magic_packet, 0xff, 6 * sizeof(unsigned char));

  unsigned char *mac_bytes = hex_mac_to_bytes(mac_hex);

  unsigned char *tmp = magic_packet;
  for (unsigned int i = 0; i < 16; i++) {
    tmp = tmp + 6 * sizeof(unsigned char);
    memcpy(tmp, mac_bytes, 6);
  }
  free(mac_bytes);

  return send(sock, &magic_packet, MAGIC_PACKET_LEN, 0);
}

int main(int argc, char *argv[]) {
  char *mac_regex = "^([0-9a-f]{2}[:-]?){5}[0-9a-f]{2}$";
  int errcode = regcomp(&VALID_MAC_RE, mac_regex, REG_EXTENDED | REG_ICASE);
  if (errcode) {
    err(EXIT_FAILURE, "regex compilation failed: %s",
        get_regerror(errcode, &VALID_MAC_RE));
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_BROADCAST;
  addr.sin_port = htons(9);

  int opt, port;
  while ((opt = getopt(argc, argv, "i:p:h")) != -1)
    switch (opt) {
    case 'i':
      if (inet_pton(AF_INET, optarg, &(addr.sin_addr.s_addr)) != 1) {
        errx(EXIT_FAILURE, "invalid broadcast address: %s", optarg);
      }
      break;
    case 'p':
      port = strtol(optarg, NULL, 10);
      if (port < 0 || port > 65535) {
        errx(EXIT_FAILURE, "invalid port: %s", optarg);
      }
      addr.sin_port = htons(port);
      break;
    default:
      usage(argv[0]);
    }

  if (optind == argc) {
    usage(argv[0]);
  }

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    err(EXIT_FAILURE, "failed to create socket");
  }
  int broadcast = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast,
                 sizeof(broadcast))) {
    err(EXIT_FAILURE, "failed to set broadcast option");
  }
  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr))) {
    err(EXIT_FAILURE, "setting default socket destination failed");
  }

  for (int i = optind; i < argc; i++) {
    int err = is_valid_mac(argv[i]);
    if (err) {
      warnx("\"%s\" is not valid MAC address: %s", argv[i],
            get_regerror(err, &VALID_MAC_RE));
      continue;
    }

    int n = send_magic_packet(sock, argv[i]);
    if (n < 0) {
      errx(EXIT_FAILURE, "failed to send magic packet");
    }
    if (n != MAGIC_PACKET_LEN) {
      errx(EXIT_FAILURE, "sent %d of %d bytes", n, MAGIC_PACKET_LEN);
    }
  }
  close(sock);
  exit(EXIT_SUCCESS);
}
