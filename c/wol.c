#include <arpa/inet.h>
#include <getopt.h>
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
          "Usage: %s [-i IP] [-p PORT] MAC...\n"
          "  -i IP      broadcast address (default: 255.255.255.255)\n"
          "  -p PORT    destination port (default: 40000)\n",
          prog);
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
  unsigned char *bytes = malloc(6 * sizeof(unsigned char));
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
  int err = regcomp(&VALID_MAC_RE, mac_regex, REG_EXTENDED | REG_ICASE);
  if (err) {
    fprintf(stderr, "Regex compilation failed: %s\n",
            get_regerror(err, &VALID_MAC_RE));
    exit(1);
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_BROADCAST;
  addr.sin_port = htons(40000);

  int opt, port;
  while ((opt = getopt(argc, argv, "i:p:h")) != -1)
    switch (opt) {
    case 'i':
      err = inet_pton(AF_INET, optarg, &(addr.sin_addr.s_addr));
      if (err != 1) {
        fprintf(stderr, "Invalid broadcast address: %s\n", optarg);
        exit(1);
      }
      break;
    case 'p':
      port = strtol(optarg, NULL, 10);
      if (port < 1 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", optarg);
        exit(1);
      }
      addr.sin_port = htons(port);
      break;
    default:
      usage(argv[0]);
      exit(2);
    }

  if (optind == argc) {
    usage(argv[0]);
    exit(2);
  }

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror("Failed to create socket");
    exit(1);
  }
  int broadcast = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast,
                 sizeof(broadcast))) {
    perror("Failed to set broadcast option");
    exit(1);
  }
  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr))) {
    perror("Error setting default socket destination");
    exit(1);
  }

  for (int i = optind; i < argc; i++) {
    int err = is_valid_mac(argv[i]);
    if (err != 0) {
      fprintf(stderr, "Invalid MAC address: %s: %s\n", argv[i],
              get_regerror(err, &VALID_MAC_RE));
      exit(1);
    }

    int n = send_magic_packet(sock, argv[i]);
    if (n < 0) {
      perror("Failed to send magic packet");
      exit(1);
    }
    if (n != MAGIC_PACKET_LEN) {
      fprintf(stderr, "WARNING: sent magic packet with wrong size (%d)\n", n);
    }
  }
  close(sock);
  return 0;
}
