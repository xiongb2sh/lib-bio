/**
 * This is a simulator developed ad-hoc to allow future optimization regardless
 * the changes to other components of the library (which are designed ti be part
 * of a library rather then efficient stand alon tools.
 */

#include "common.hpp"

#include <cstdlib>
#include <ctime>

#include <iostream>
#include <algorithm>


char bases[] = {'A', 'C', 'G', 'T'};
char revBases[128];
Options Options::opts;

// output quantities (common to online and offline)
double p_fail = 0.0;
size_t holes = 0;
size_t actually_produced_reads = 0;
double scoreSum = 0.0;
EmpiricalDistribution scoreDist(0,1,10);

// output quantities for oracle simulations

/* 
   An estimation point contains for each s=0,...,m
     - sum of all scores
     - sum of all denominators
     - sum of all denominators
     - number of observations of 's'
   This can then be used to compute average quantities and compare
   them (i.e., esitmated) with the theoretical one (possibly
   approximated).
*/ 
struct EstimationPoint
{  
  double sumNum;
  double sumDen;
  double sumScore;
  int count;
  int hammDist;
};
EstimationPoint * oraclePoints;

EditDistanceSimOutput* edOut;

void initSimulator() {
  revBases['A'] = revBases['a'] = 0;
  revBases['C'] = revBases['c'] = 1;
  revBases['G'] = revBases['g'] = 2;
  revBases['T'] = revBases['t'] = 3;
  scoreDist = EmpiricalDistribution(0,1,Options::opts.empiricalDistributionStep);
  initRandomGenerator();
  initUtil();
  initProbabilities();
  initChainMatrix();
  // init for oracle points
  oraclePoints = new EstimationPoint[Options::opts.m + 1];
  
  for (size_t i = 0; i < Options::opts.m; ++i) {
    oraclePoints[i].sumNum = 0.0;
    oraclePoints[i].sumDen = 0.0;
    oraclePoints[i].sumScore = 0.0;
    oraclePoints[i].count = 0;
    oraclePoints[i].hammDist = 0;
  }

  // set the precision from options
  std::cout.precision(Options::opts.floatPrecision);

  edOut = new EditDistanceSimOutput();
}

void clearSimulator() {
  delete edOut;
  delete[] oraclePoints;
  clearChainMatrix();
  clearProbabilities();
  clearUtil();
}

void outputResults() {
  if (Options::opts.mode == OpMode::EditDist) {
    // CDF ouput requested (-D <distfile> option)
    if (!Options::opts.outputDistribution.empty() &&
	Options::opts.subTask != EDIT_DISTANCE_SUBTASK_SCRIPT_DIST) {
      if (edOut->distPDF) {
	size_t n = Options::opts.N;	
	std::ofstream ofs(Options::opts.outputDistribution, std::ofstream::out);      
	logInfo("Writing Edit Distance distribution on " + Options::opts.outputDistribution);
	for (size_t i = 0; i <= n; ++i) {
	  ofs << edOut->distPDF[i] << std::endl;
	}
	ofs.close();
      }
    }
    return;
  }
  if (Options::opts.pipeline) {
    if (Options::opts.mode == OpMode::Oracle) {
      double appNumDen[2];
      for (size_t i = 0; i < Options::opts.m+1; ++i) {
	approximatedScore(i, appNumDen);
	std::cout << i << "\t" <<
	  oraclePoints[i].sumScore << "\t" << oraclePoints[i].count << "\t" <<
	  oraclePoints[i].sumNum << "\t" << oraclePoints[i].sumDen << "\t" <<
	  appNumDen[0] << "\t" << appNumDen[1] << "\t" << oraclePoints[i].hammDist << "\n";
      }
      return;
    }
    std::cout << p_fail << std::endl;
  } else {
    std::cout << "P[Fail]    = " << p_fail << std::endl;
    std::cout << "P[Success] = " << 1.0 - p_fail << std::endl;
    std::cout << "#[Holes]   = " << holes << std::endl;
    std::cout << "#[Reads]   = " << actually_produced_reads << std::endl;
  }
  if (!Options::opts.outputDistribution.empty()) {
    std::ofstream ofs(Options::opts.outputDistribution, std::ofstream::out);
    for (size_t i = 0; i < scoreDist.getIntervalCount(); ++i) {
      ofs << scoreDist.valueAtIndex(i) << '\n';
    }
    ofs.close();
  }

  if (!Options::opts.outputCDF.empty()) {
    std::ofstream ofs(Options::opts.outputCDF, std::ofstream::out);
    std::vector<double> cdf(scoreDist.getIntervalCount());
    scoreDist.getCDF(cdf);
    for (size_t i = 0; i < scoreDist.getIntervalCount(); ++i) {
      ofs << cdf[i] << "\n";
    }
    ofs.close();
    std::cout << scoreDist.valueAtIndex(percentileIndex(cdf,0.001)) << "\n";
  }
}

void recordScoreWithOverlap(double sc, size_t s) {
}

void recordScore(double p_ab) {
  p_fail += 1.0 - p_ab;
  scoreSum += p_ab;
  scoreDist.addSample(p_ab);
}

void offlineSimulation() {

  char* ref = NULL;
  
  size_t N = Options::opts.N;
  size_t m = Options::opts.m;
  
  ref = new char[N];
  generateIIDGenome(N,ref);
  std::string s(ref);

  // priority queue is used with position as key so that while extractin reads at
  // once (i.e., emptying the queue) reads will be presented in ordered by
  // position on the reference sequence
  std::priority_queue<Read> reads;
  generateOfflineReads(s, reads);

  // Temporary variables to count the number of holes, in the future a more
  // sophisticated way (e.g., finite state machine) should be used.
  bool onHole = false;
  
  Read r1 = reads.top();
  reads.pop();
  while(!reads.empty()) {
    Read r2 = reads.top();
    reads.pop();
    size_t s = m - (r2.j - r1.j);
    evaluateChainRelation(r1, r2, s);

    if (s <= m) {
      onHole = false;
      double p_ab = randomReadsOverlapProbNoErr(r1.r,r2.r,s);
      recordScore(p_ab);

    } else {
      if (onHole == false) {
	onHole = true;
	holes++;
      }
      addNonOverlapRecord(r2.j - r1.j - m);
      double x = (double)Options::opts.N - 2.0 * (double)Options::opts.m + 1.0
	+ overlappingStringsSum(r1.r, r2.r);
      recordScore(1.0 / x);
    }
    r1 = r2;
  }
  
  delete[] ref;
}

void onlineSimulation() {

  bool onHole = false;

  size_t N = Options::opts.N;
  size_t m = Options::opts.m;
  
  GenomeSegment g(N, m, MAX_GENOME_SEGMENT_LENGTH);
  generateFirstGenomeSegment(g);

  size_t generated_reads = 0;
  size_t current_position = 0;
  size_t real_position = 0;
  size_t remaining_genome = g.length;

  size_t actual_M = 0;

  Read prev_read("", -1);
  
  while (real_position < N - m) {

    size_t d = generateInterReadDistance();
    real_position += d;

    // this is artificial however for reasonable values of parameters it should
    // never happen otherwise we woul need a different way of online generating
    // the genome.
    // More specifically if that happens it means that 'd' is higher then a whole
    // genome segment (which should be no less than 10000 in practical cases) for
    // reasonable values of N and M this event will have probability zero for
    // all practical situations and artifically skipping over such 'extreme' values
    // of d will not appreciably change final results
    if (d > (g.length - m - 1)) {
      continue;
    }

    // in this case we need to generate new genome segment...
    if (remaining_genome < m + d) {
      size_t tmp = current_position + d;
      if (tmp < g.length) {
	generateNewGenomeSegment(g, g.length - tmp);
      } else {
	generateNewGenomeSegment(g, 0);
      }
      current_position = 0;
      remaining_genome = g.length;
    } else {
      // ...otherwise we simply update counters
      current_position += d;    
      remaining_genome -= d;
    }
   
    Read current = generateOnlineRead(g.genome,current_position);
    actual_M++;
    current.j = real_position;

    // here the probabilities are computed and accumulated
    if (prev_read.j != -1) {       
      
      if (d > m) {
	if (!onHole) {
	  holes++;
	}
	onHole = true;
	// non-overlap case...
	if (Options::opts.approxLevel < 0) {
	  double sc = score(prev_read.r, current.r , 0);	  
	  recordScore(sc);
	}
		
      } else {
	onHole = false;
	// overlap case...
	size_t s = m - d;
	double sc = score(prev_read.r, current.r , s);
	recordScore(sc);
      }
    }
    
    generated_reads++;
    current.j = real_position;
    prev_read = current;
  }
  actually_produced_reads = actual_M;
}

void oracleSimulation() {  
  size_t n = 2 * Options::opts.m;
  int m = Options::opts.m;
  double alpha = 1.0 / ((double)Options::opts.N - 2.0 * m + 1);
  double numDen[2];
  
  char* genome = new char[n];  
  // Oracle simulation loops to produce exactly M-1 consecutive pairs
  for (size_t i = 0; i < Options::opts.M - 1; ++i) {
    // generate 2m bases of genome
    generateIIDGenome(n, genome);

    // generate first reads at position 0
    Read r1 = generateOnlineRead(genome, 0);
    // generate inter-arrival d
    size_t d = generateInterReadDistance();

    // generate second reads at position d
    if (d >= m) {      
      oraclePoints[0].sumNum += 1.0;
      oraclePoints[0].sumDen += (1.0 / alpha);
      oraclePoints[0].sumScore += alpha;
      oraclePoints[0].count++;
    } else {
      Read r2 = generateOnlineRead(genome, d);
      size_t s = m - d;
      oraclePoints[s].hammDist += prefixSuffixHammingDistance(r1.r, r2.r, s);
      double sc = scoreExt(r1.r, r2.r, s,numDen);
      oraclePoints[s].sumNum = numDen[1];
      oraclePoints[s].sumDen = numDen[0];
      oraclePoints[s].sumScore += sc;
      oraclePoints[s].count++;
      
    }
  }
  delete[] genome;
}

void
editDistanceOpMode() {
  // The default edit distance mode is
  // Sample
  // Linear Alg
  // mean and variance output
  // no script
  // no sample matrix
  int flags = Options::opts.optFlags;
  size_t n = Options::opts.N;
  int task = Options::opts.subTask;
  edOut->distPDF = new double[n+1];
  std::fill_n(edOut->distPDF, n+1, 0);

  if (task == EDIT_DISTANCE_SUBTASK_SCRIPT_DIST) {
    size_t** freqMat = allocMatrix<size_t>(n+1, n+1);
    std::vector<std::string>* allScripts = nullptr;
    if (!Options::opts.outputDistribution.empty()) {
      allScripts = new std::vector<std::string>();
    }
    scriptDistributionMatrix(n, n, Options::opts.k, freqMat, allScripts);
    freeMatrix<size_t>(n+1, n+1, freqMat);
    if (allScripts != nullptr) {
      std::ofstream ofs(Options::opts.outputDistribution, std::ofstream::out);
      for (std::string script : *allScripts) {
	ofs << script << "\n";
      }
      ofs.close();
      delete allScripts;
      allScripts = nullptr;
    }
    return;
  }

  if (flags & EDIT_DISTANCE_BOUNDED_ERROR) {
    size_t k_max = Options::opts.k;
    double precision = Options::opts.precision;
    double z_confidence = Options::opts.confidence;
    std::vector<SampleEstimates> est = differenceBoundedRelativeErrorEstimate(n, precision, z_confidence, k_max);
    std::cout << std::endl;
    std::cout << (n>>1) << "\t" << est[0].sampleSize << "\t"
	      << est[0].sampleMean  << "\t" << est[0].sampleVariance << "\n";    
    std::cout << n << "\t" << est[1].sampleSize
	      << "\t" << est[1].sampleMean  << "\t" << est[1].sampleVariance << "\n";
    return;
  }
  
  if (flags & EDIT_DISTANCE_ESTIMATE_EXHAUSTIVE) {
    // Exhasutve (only quadratic)
    print_warning("only \033[1;37mqudratic algorithm\033[0m available with exhaustive option");
    double avgDist = testExhaustiveEditDistanceEncoded(n, edOut->distPDF);
    std::cout << avgDist << std::endl;    
  }
  
  else {
    // SAMPLE
    size_t k = Options::opts.k;

    if (flags & EDIT_DISTANCE_ALGORITHM_QUADRATIC) {
      // QUADRATIC + Sample
      if (flags & EDIT_DISTANCE_INFO_PARTIAL) {
	// Sample + Quadratic + Partial Info
	if (flags & EDIT_DISTANCE_INFO_SCRIPT) {
	  std::unique_ptr<EditDistanceInfo[]> infos =
	    editDistSamplesInfo(n,k);
	  for (size_t i = 0; i < k; ++i) {
	    std::cout << infos[i].n_sub << "\t" << infos[i].n_ins << "\t" << infos[i].n_del << "\n";
	  }
	}
	else {
	  print_warning("Quadratic info without script not yet implemented");
	}
      }
    }

    
    else {
      // LINEAR + Sample
      if (flags & EDIT_DISTANCE_INFO_PARTIAL) {
	// PARTIAL INFO + Sample + Linear
	print_warning("Partial info for linear under developement");
	std::unique_ptr<EditDistanceInfo[]> samples =
	  editDistSamplesInfoLinSpace(n,k);
	
	std::unique_ptr<double[]> subSamples = extractSubstitutionArray(samples.get(), k);
	std::unique_ptr<double[]> delSamples = extractDeletionArray(samples.get(), k);
	std::unique_ptr<double[]> insSamples = extractInsertionArray(samples.get(), k);
	SampleEstimates subEst = estimatesFromSamples<double>(subSamples.get(), k);
	SampleEstimates delEst = estimatesFromSamples<double>(delSamples.get(), k);
	SampleEstimates insEst = estimatesFromSamples<double>(insSamples.get(), k);

	// If 'verbose' is set  all samples are printed
	if (Options::opts.verbose) {
	  for (size_t i = 0; i < k; ++i) {
	    std::cout << samples[i] << std::endl;
	  }
	}

	std::cout << subEst.sampleMean << "\t" << subEst.sampleVariance << "\n";
	std::cout << delEst.sampleMean << "\t" << delEst.sampleVariance << "\n";
	std::cout << insEst.sampleMean << "\t" << insEst.sampleVariance << "\n";
	
      }
      else {
	// MINIMAL INFO (mean + var) + Sample + Linear
	std::unique_ptr<size_t[]> samples =
	  editDistSamples(n,k);
	SampleEstimates estimators = estimatesFromSamples<size_t>(samples.get(), k);
	std::cout << estimators.sampleMean << std::endl;
	std::cout << estimators.sampleVariance << std::endl;
      }
    }
  }
}

int main(int argc, char** argv) {   
  // Important NOT invert (init requires argument to be parsed)
  parseArguments(argc,argv);
  initSimulator();

  switch (Options::opts.mode) {
  case (OpMode::Test):
    testAll();
    exit(0);
  case (OpMode::Offline):
    offlineSimulation();
    break;
  case (OpMode::Online):
    onlineSimulation();
    break;
  case (OpMode::Oracle):
    oracleSimulation();
    break;
  case (OpMode::AlignScore):
    evaluateAlignmentScore(Options::opts);
    break;
  case (OpMode::EditDist):
    editDistanceOpMode();
    break;
  default:
    std::cout << "Unrecognized operation mode " <<
      Options::opts.mode << "\nAborting..\n";
    exit(1);
  }
  outputResults();
  clearSimulator();
  
  return 0;
}
