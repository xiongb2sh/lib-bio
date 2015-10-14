#include "common.h"

#include <getopt.h>
#include <cstdlib>
#include <iostream>

void printUsage() {
  std::cout << "USAGE\n";
  std::cout << "\tsimulator -N [ref_len] -m [read_len] -M [read_count]\n\n";
}

void printArguments() {
  std::cout << "\t\t    +++++  ARGUMENTS +++++\n\n";
  std::cout << "Reference length (N)         " << Options::opts.N << std::endl;
  std::cout << "Reads length (m)             " << Options::opts.m << std::endl;
  std::cout << "Reads count (M)              " << Options::opts.M << std::endl;
}

void parseArguments(int argc, char** argv) {
  Options::opts.N = 1000000; // size of the reference;
  Options::opts.m = 50;
  Options::opts.M = 10;
  Options::opts.verbose = false;
  char c;
  while ((c = getopt(argc, argv, "N:m:M:hv")) != -1) {
    switch(c) {
    case 'N':
      Options::opts.N = atoi(optarg);
      break;
    case 'M':
      Options::opts.M = atoi(optarg);
      break;
    case 'm':
      Options::opts.m = atoi(optarg);
      break;
    case 'v':
      Options::opts.verbose = true;
      break;
    case 'h':
      printUsage();
      exit(0);
    default:
      printUsage();
      exit(1);
    }
  }

  if (Options::opts.verbose) {
    printArguments();
  }
}
