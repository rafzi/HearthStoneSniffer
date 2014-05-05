#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

#include <pcap.h>

void print_usage(char *name, FILE *out) {
  fprintf(out, "Usage: %s [-i iface] [-o outfile] <filter expression>\n", name);
}

void handler(uint8_t *user, const struct pcap_pkthdr *header, const uint8_t *packet) {
  if (header->caplen < header->len) {
    fprintf(stderr, "truncated packet (%d of %d bytes)\n", header->caplen, header->len);
  } else {
    fprintf(stderr, "capture packet (%d bytes)\n", header->caplen);
  }

  pcap_dump(user, header, packet);
}

struct {
  char *device;
  char *error;
  char *outfile;
  char *exp;
  int showhelp;
} opts;

void parse_opts (char *argv[]) {
  char *cur = *argv;
  char *next = NULL;

  while (cur != NULL) {
    next = *(++argv);

    if (strcmp(cur, "-i") == 0) {
      if (next == NULL) {
        opts.error = "Options -i requires an arguments!";
        return;
      }

      opts.device = next;
      next = *(++argv);
    } else if (strcmp(cur, "-o") == 0) {
      if (next == NULL) {
        opts.error = "Options -o requires an argument!";
        return;
      }
      opts.outfile = next;
      next = *(++argv);
    } else {
      if (opts.exp == NULL) {
        opts.exp = cur;
      } else {
        opts.error = "Trailing positional argument";
        return;
      }
    }
    cur = next;
  }
  if (opts.exp == NULL) {
    opts.error = "filter expression is required";
  }
}

int 
main(int argc, char *argv[]) {
  opts.device = NULL;
  opts.error = NULL;
  opts.outfile = NULL;
  opts.exp = NULL;
  opts.showhelp = 0;

  pcap_dumper_t *pd;

  // parse options
  parse_opts(argv+1);

  if (opts.error) {
    fprintf(stderr, "Error: %s\n", opts.error);
        print_usage(argv[0], stderr);
    exit(1);
  }

  // Initialize pcap
  pcap_if_t *alldevs;
  char errbuf[PCAP_ERRBUF_SIZE];

  if (pcap_findalldevs(&alldevs, errbuf) == -1) {
    fprintf(stderr, "pcap_findalldevs: %s\n", errbuf);
    return 1;
  }

  for (pcap_if_t* dev = alldevs; dev != NULL; dev = dev->next) {
	fprintf(stderr, "find interface: %s\n", dev->name);	
    if (!strcmp(opts.device, dev->name)) {
      fprintf(stderr, "find interface: %s\n", dev->name);
      break;
    }
  }

  pcap_freealldevs(alldevs);

  pcap_t *pcap = pcap_open_live(opts.device, 65535, 0, 1000, errbuf);
  if (!pcap) {
    fprintf(stderr, "pcap_open_live(%s): %s\n", opts.device, errbuf);
    return 1;
  } 

  struct bpf_program bpf;
  if (pcap_compile(pcap, &bpf, opts.exp, 1, 0) == -1) {
    fprintf(stderr, "pcap_compile(%s): %s\n", opts.exp, pcap_geterr(pcap));
    return 1;
  }

  if (pcap_setfilter(pcap, &bpf) == -1) {
    fprintf(stderr, "pcap_setfilter(%s): %s\n", opts.exp, pcap_geterr(pcap));
    return 1;
  }

  if ((pd = pcap_dump_open(pcap, opts.outfile)) == NULL) {
    fprintf(stderr, "error opening savefile \"%s\" for writing: %s\n", opts.outfile, pcap_geterr(pcap));
    return 1;
  }
  
  if (pcap_loop(pcap, -1, handler, (uint8_t *)pd) < 0) {
    fprintf(stderr, "pcap_loop: %s\n", pcap_geterr(pcap));
  }

  fprintf(stderr, "pcap_loop exited");

  pcap_dump_close(pd);
  pcap_close(pcap);

  return 0;
}
