#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <pcap.h>

void print_usage(char *name, FILE *out) {
  fprintf(out, "Usage: %s <savefile>\n", name);
}

void handler(uint8_t *user, const struct pcap_pkthdr *header, const uint8_t *packet) {
  if (header->caplen < header->len) {
    fprintf(stderr, "truncated packet (%d of %d bytes)\n", header->caplen, header->len);
  } else {
    fprintf(stderr, "capture packet (%d bytes)\n", header->caplen);
  }
}

int
main(int argc, char *argv[]) {
  char *file;

  if (argc == 1) {
    print_usage(argv[0], stderr);
    return 1;
  } else { 
    file = argv[1];
  }

  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t *pcap = pcap_open_offline(file, errbuf);
  if (!pcap) {
    fprintf(stderr, "pcap_open_offline(%s): %s\n", file, errbuf);
    return 1;
  }

  struct pcap_pkthdr *header;
  const uint8_t *packet;
  int ret;
  while ((ret = pcap_next_ex(pcap, &header, &packet)) == 1) {
    handler(NULL, header, packet);
  }

  pcap_close(pcap);

  return 0;
}

