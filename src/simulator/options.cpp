#include "common.hpp"

#include <getopt.h>
#include <cstdlib>
#include <iostream>

void printUsage() {
  std::cout << std::endl;
  std::cout << "USAGE\n";
  std::cout << "\tsimulator [<OPTIONS>]\n\n";
  std::cout << " OPTIONS\n\n";
  std::cout << "\t-N <length>   Length of reference sequence\n";
  std::cout << "\t-M <length>   Length of reads  [1,2,...]\n";
  std::cout << "\t-m <count>    Number of reads  [1,2,...]\n";
  std::cout << "\t-e <error>    Error probability for base call [0.0,1.0]\n";
  std::cout << "\t-i <path>     Path of a fasta file for the reference\n";
  std::cout << "\t-D <path>     Path of a output file for distribution of scores\n";
  std::cout << "\t-C <path>     Path of a output file for CDF (cumulative distribution) of scores\n";
  std::cout << "\t-A <approx>   Set approximation levels (default no approx)\n";
  std::cout << "\t-O <op_mode>  Set operation mode of the simulator [0,4] (run -h for details)\n";
  std::cout << "\t-o            Executes online generation of reference. Deprecated use -O 2 instead\n";
  std::cout << "\t-p            Outputs on standard out for pipelining\n";
  std::cout << "\t-h            Shows extended help\n";
  std::cout << "\t-v            Activate verbose mode\n";
  std::cout << "\t-T            perform test (every other option is ignored). Deprecated use -O 0 instead\n";
  std::cout << std::endl;
}

void printArguments() {
  std::cout << "\t\t    +++++  ARGUMENTS +++++\n\n";
  std::cout << "Reference length (N)         " << Options::opts.N << std::endl;
  std::cout << "Reads length (m)             " << Options::opts.m << std::endl;
  std::cout << "Reads count (M)              " << Options::opts.M << std::endl;
  std::cout << "Base error probability (pe)  " << Options::opts.pe << std::endl;
}

void printOperationModeDescription() {
  std::cout << " OPERATION MODES\n\n";
  std::cout << "\t 0 (Test)     Performs several tests of quantities\n";
  std::cout << "\t 1 (Offline)  Offline simulation: all genome and reads generated before scoring (Defualt)\n";
  std::cout << "\t 2 (Online)   Online simulation: genome and reads are generated 'on-demand'\n";
  std::cout << "\t 3 (Oracle)   Simulation of M-1 independent pairs\n";
  std::cout << "\t 4 (ScoreEst) Tests the approximation E[score] ~= E[num]/E[den]\n\n";
}

void parseArguments(int argc, char** argv) {

  // default options
  Options::opts.N = 1000000; // size of the reference;
  Options::opts.m = 50;
  Options::opts.M = 10;
  Options::opts.pe = 0.01;
  Options::opts.empiricalDistributionStep = 100;
  Options::opts.inputReference = "";
  Options::opts.outputDistribution = "";
  Options::opts.outputCDF = "";
  Options::opts.approxLevel = -1;

  Options::opts.mode = OpMode::Offline;
  Options::opts.online = false;
  Options::opts.pipeline = false;
  Options::opts.verbose = false;
  Options::opts.test = false;
  
  char c;
  while ((c = getopt(argc, argv, "N:m:M:e:i:D:C:A:O:ophvT")) != -1) {
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
    case 'e':
      Options::opts.pe = atof(optarg);
      break;
    case 'i':
      Options::opts.inputReference = optarg;
      break;
    case 'D':
      Options::opts.outputDistribution = optarg;
      break;
    case 'C':
      Options::opts.outputCDF = optarg;
      break;
    case 'A':
      Options::opts.approxLevel = atoi(optarg);
      break;
    case 'o':
      Options::opts.online = true;
      Options::opts.mode = OpMode::Online;
      break;
    case 'O':
      Options::opts.mode = static_cast<OpMode>(atoi(optarg));
      break;
    case 'p':
      Options::opts.pipeline = true;
      break;
    case 'v':
      Options::opts.verbose = true;
      break;
    case 'T':
      Options::opts.test = true;
      Options::opts.mode = OpMode::Test;
      break;
    case 'h':
      printUsage();
      printOperationModeDescription();
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
