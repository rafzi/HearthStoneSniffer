#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <pcap.h>

#include <net/ethernet.h>
#include <linux/ip.h>
#include <linux/tcp.h>

#include <arpa/inet.h>

void print_usage(char *name, FILE *out) {
  fprintf(out, "Usage: %s <savefile>\n", name);
}


int extract_payload(const struct pcap_pkthdr *header, const uint8_t *packet, const uint8_t **payload) {
  struct ether_header *ethh;
  struct iphdr *iph;
  struct tcphdr *tcph;

  ethh = (struct ether_header *)packet;
  if (ntohs(ethh->ether_type) != ETHERTYPE_IP) {
    fprintf(stderr, "Not an IP packet.\n");
    return -1;
  }

  iph = (struct iphdr *)(packet + sizeof(struct ether_header));
  if (ntohs(iph->tot_len) == 40) {
    fprintf(stderr, "No Payload.\n");
    *payload = NULL;
    return -1;
  }
  
  tcph = (struct tcphdr *)((uint8_t *)iph + sizeof(struct iphdr));

  *payload = ((uint8_t *)tcph + sizeof(struct tcphdr));

  return 0;
}

void handler(uint8_t *user, const struct pcap_pkthdr *header, const uint8_t *packet) {
  if (header->caplen < header->len) {
    fprintf(stderr, "truncated packet (%d of %d bytes)\n", header->caplen, header->len);
  } else {
    //    fprintf(stderr, "capture packet (%d bytes)\n", header->caplen);
  }

  const uint8_t *payload;
  const uint32_t *temp;
  int ret;

  if((ret = extract_payload(header, packet, &payload)) == 0) {
    temp = (const uint32_t *)payload;
    fprintf(stderr, "Payload type: %d, length: %d.\n", temp[0], temp[1]);
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

