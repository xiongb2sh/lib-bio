#include <include/common.hpp>
#include <include/edit.hpp>

#include <include/options.hpp>

#include <include/prob.hpp>
#include <include/generator.hpp>
#include <include/util.hpp>


#include <fstream>
#include <map>
#include <cmath>

using namespace lbio::sim::generator;


//////////////////////////////////////////////////////////////////////
//                    EDIT INFO IMPLE AND HELPERS
//////////////////////////////////////////////////////////////////////

std::unique_ptr<double[]>
extractSubstitutionArray(const EditDistanceInfo* v, size_t k) {
  std::unique_ptr<double[]> o(new double[k]);
  for (size_t i = 0; i < k; ++i) {
    o[i] = v[i].n_sub;
  }
  return o;
}


std::unique_ptr<double[]>
extractDeletionArray(const EditDistanceInfo* v, size_t k) {
  std::unique_ptr<double[]> o(new double[k]);
  
  for (size_t i = 0; i < k; ++i) {
    o[i] = v[i].n_del;
  }
  return o;
}


std::unique_ptr<double[]>
extractInsertionArray(const EditDistanceInfo* v, size_t k)  {
  std::unique_ptr<double[]> o(new double[k]);
  for (size_t i = 0; i < k; ++i) {
    o[i] = v[i].n_ins;
  }
  return o;
}

EditDistanceInfo::EditDistanceInfo()
  : n_sub {0}, n_del {0}, n_ins {0}, edit_script {""}
{ }

EditDistanceInfo::EditDistanceInfo(lbio_size_t s, lbio_size_t d, lbio_size_t i)
  : n_sub {s}, n_del {d}, n_ins {i}, edit_script {""}
{ }

EditDistanceInfo::EditDistanceInfo(lbio_size_t c)
  : EditDistanceInfo(c,c,c)
{ } 

size_t
EditDistanceInfo::distance() const {
  return n_sub + n_ins + n_del;
}
  
void
EditDistanceInfo::reset() {
  n_sub = 0; n_del = 0; n_ins = 0;
}

bool
EditDistanceInfo::operator<(const EditDistanceInfo& i) const {
  return (this->distance() < i.distance());
}
  
bool
EditDistanceInfo::operator==(const EditDistanceInfo& i) const {
  return (n_sub == i.n_sub && n_del == i.n_del && n_ins == i.n_ins);
}
  
bool
EditDistanceInfo::operator!=(const EditDistanceInfo& i) const {
  return !(*this == i);
}
  
EditDistanceInfo&
EditDistanceInfo::operator+=(const EditDistanceInfo& rhs) {
  n_sub += rhs.n_sub;
  n_del += rhs.n_del;
  n_ins += rhs.n_ins;
  return *this;
}

EditDistanceInfo
operator+(EditDistanceInfo lhs, const EditDistanceInfo& rhs) {
  lhs += rhs;
  return lhs;
}

EditDistanceInfo&
EditDistanceInfo::operator*=(lbio_size_t scalar) {
  n_sub *= scalar;
  n_del *= scalar;
  n_ins *= scalar;
  return *this;
}

EditDistanceInfo
operator*(EditDistanceInfo lhs, lbio_size_t rhs) {
  lhs *= rhs;
  return lhs;
}

 
std::ostream&
operator<<(std::ostream& out, const EditDistanceInfo& info) {
  out << info.n_sub << " " << info.n_del << " " << info.n_ins;
  return out;
}  

//////////////////////////////////////////////////////////////////////
//                      EDIT DISTANCE COMPUTATION
//////////////////////////////////////////////////////////////////////

/**
 * \brief Conputes the edit distance between strings s1 and s2 using only
 * linear space (the vecotors passed as parameters). Vectors must be at
 * least m+1 long where m is the length of the second string s2
 */
size_t
editDistanceLinSpace(const std::string& s1, const std::string& s2,
		     size_t* v0, size_t* v1) {
  size_t n1 = s1.size();
  size_t n2 = s2.size();
  size_t n_max = std::max(n1, n2);
  for (size_t i = 0; i < n_max+1; ++i) {
    v0[i] = i;
  }

  for (size_t i = 1; i <= n1; ++i) {
    v1[0] = i;
    for (size_t j = 1; j <= n2; ++j) {
      size_t delta = (s1[i-1] == s2[j-1]) ? 0 : 1;
      v1[j] = std::min(std::min(v0[j]+1, v1[j-1]+1), v0[j-1]+delta);
    }
    size_t * tmp = v0;
    v0 = v1;
    v1 = tmp;
  }
  return v0[n2];
}

EditDistanceInfo
editDistanceLinSpaceInfo(const std::string& s1, const std::string& s2,
			 EditDistanceInfo* v0, EditDistanceInfo* v1,
			 EditDistanceInfo** sampleMat) {
  size_t n1 = s1.size();
  size_t n2 = s2.size();
  size_t n_max = std::max(n1, n2);

  EditDistanceInfo* tmp = NULL;

  for (size_t i = 0; i <= n_max; ++i) {
    v0[i].n_ins = i;
    v0[i].n_del = 0;
    v0[i].n_sub = 0;
  }

  for (size_t i = 1; i <= n1; ++i) {
    v1[0].n_ins = 0;
    v1[0].n_del = i;
    v1[0].n_sub = 0;
    for (size_t j = 1; j <= n2; ++j) {
      size_t delta = (s1[i-1] == s2[j-1]) ? 0 : 1;
      size_t a = v0[j-1].distance() + delta; // a = M[i-1][j-1] + delta
      size_t b = v0[j].distance() + 1;       // b = M[i-1][j]   + 1     
      size_t c = v1[j-1].distance() + 1;     // c = M[i][j-1]   + 1

      // Case of match
      if (s1[i-1] == s2[j-1]) {
	v1[j] = v0[j-1];
	//	continue;
      } else {
	// Case of substitution NOT WORSE than others
	if ( (a <= b) && (a <= c)) {
	  v1[j] = v0[j-1];
	  v1[j].n_sub++;	
	} else {
	  // In case of tie (equality) always select a deletion
	  if ( b <= c ) {
	    v1[j] = v0[j];
	    v1[j].n_del++;
	  } else {
	    v1[j] = v1[j-1];
	    v1[j].n_ins++;
	  }
	}
      }
      if (sampleMat != NULL) {
	sampleMat[i-1][j-1] += v1[j];
      }
    }
    
    tmp = v0;
    v0 = v1;
    v1 = tmp;

  }
  return v0[n2];
}
void
editInfoCompute(EditDistanceInfo& info) {
  info.n_sub = 0;
  info.n_ins = 0;
  info.n_del = 0;
  for (char c : info.edit_script) {
    if (c == 'I') {
      info.n_ins++;
    }
    if (c == 'D') {
      info.n_del++;      
    }
    if (c == 'S') {
      info.n_sub++;
    }
  }
}

//////////////////////////////////////////////////////////////////////
//                    EDIT DISTANCE APPROXIMATIONS
//////////////////////////////////////////////////////////////////////

void
editDistanceBandwiseApproxMat(const std::string& s1, const std::string& s2,
			      size_t T, size_t** dpMatrix) {
  size_t n = s1.size();
  size_t m = s2.size();
  size_t INF = n+m+1;
  // init matrix assuming T < min{n,m} (the strictness is crucial to
  // initialize the 'border' diagonals  
  dpMatrix[0][0] = 0;	   
  for (size_t i = 1; i <= T; ++i) {
    dpMatrix[i][0] = i;
  }
  for (size_t j = 1; j <= T; ++j) {
    dpMatrix[0][j] = j;
  }

  for (size_t t = 0; t <= m - (T+1); ++t) {
    dpMatrix[t][T+1+t] = INF;
  }

  for (size_t t = 0; t <= n - (T+1); ++t) {
    dpMatrix[T+1+t][t] = INF;
  }  

  for (size_t i = 1; i <= n; ++i) {
    size_t j_min = (size_t)std::max<int>(1, (int)(i-T));
    size_t j_max = (size_t)std::min<int>(m, (int)(i+T));
    for (size_t j = j_min; j <= j_max; ++j) {      
      size_t delta = (s1[i-1] == s2[j-1]) ? 0 : 1;
      dpMatrix[i][j] = std::min( dpMatrix[i-1][j-1] + delta,
				 std::min(dpMatrix[i-1][j] + 1,
					  dpMatrix[i][j-1] + 1));
    }
  }
}

size_t
editDistanceBandwiseApprox(const std::string& s1, const std::string& s2,
			   size_t T) {
  size_t n = s1.size();
  size_t m = s2.size();
  size_t** dpMatrix = allocMatrix<size_t>(n+1, m+1);
  editDistanceBandwiseApproxMat(s1, s2, T, dpMatrix);  
  size_t dist = dpMatrix[n][m];
  freeMatrix<size_t>(n+1, m+1, dpMatrix);
  return dist;
}


//////////////////////////////////////////////////////////////////////
//                       EDIT DISTANCE SAMPLING
//////////////////////////////////////////////////////////////////////


std::unique_ptr<EditDistanceInfo[]>
editDistSamplesInfoLinSpace(size_t n, size_t k_samples,
			    EditDistanceInfo** sampleMat) {
  std::unique_ptr<EditDistanceInfo[]> samples(new EditDistanceInfo[k_samples]);
  std::string s1(n, 'N');
  std::string s2(n, 'N');

  EditDistanceInfo* v0 = new EditDistanceInfo[n+1];
  EditDistanceInfo* v1 = new EditDistanceInfo[n+1];

  for (size_t k = 0; k < k_samples; ++k) {
    generateIIDString(s1);
    generateIIDString(s2);
    samples[k] = editDistanceLinSpaceInfo(s1,s2, v0, v1, sampleMat);
  }

  delete[] v1;
  delete[] v0;
  
  return samples;
}

std::vector<size_t>
edit_samples_fixed_string(size_t n, size_t k_samples, const std::string& s2) {
  std::vector<size_t> samples;
  std::string s1(n, 'N');

  size_t* v0 = new size_t[n+1];
  size_t* v1 = new size_t[n+1];

  for (size_t k = 0; k < k_samples; ++k) {
    generateIIDString(s1);
    samples.push_back(editDistanceLinSpace(s1,s2, v0, v1));
  }

  delete[] v1;
  delete[] v0;
  
  return samples;
}

// ----------------------------------------------------------------------
//                          ESTIMATION PROCEDURE
// ----------------------------------------------------------------------

SampleEstimates
editDistanceErrorBoundedEstimates(size_t n, double precision,
				  double z_delta, size_t k_min) {
  using BandApprox = EditDistanceBandApproxLinSpace<lbio_size_t, std::string>;
  BandApprox alg(n, n, std::floor(n/2.0), {1,1,1});
  EditDistanceSample<BandApprox> gen(n, n);
  
  size_t k = 1;
  size_t k_max = Options::opts.k;
  size_t sample = gen(alg); 
  double mean_k = sample;
  double var_k = 0;

  double cumulative_sum = sample;
  double cumulative_quad_sum = sample * sample;
  
  while(k < k_max) {
    k++;
    sample = gen(alg);
    cumulative_sum += sample;
    cumulative_quad_sum += (sample * sample);
    mean_k = cumulative_sum / ((double)k);
    var_k = ( cumulative_quad_sum - k*(mean_k*mean_k)  ) / ((double)(k-1));
    if  ( (var_k * ( z_delta*z_delta ) < ((double)k) * ( precision * precision ))
	  && (k > k_min) ){
      break;
    }
  }

  SampleEstimates est;
  est.sampleSize = k;
  est.sampleMean = mean_k;
  est.sampleVariance = var_k;

  return est;
}

SampleEstimates
editDistanceRelativeErrorEstimates(size_t n, double e_model,
				   double precision, double z_delta) {

  using BandApprox = EditDistanceBandApproxLinSpace<lbio_size_t, std::string>;
  BandApprox alg(n, n, std::floor(n/2.0), {1,1,1});
  EditDistanceSample<BandApprox> gen(n, n);

  
  size_t k = 1;
  size_t k_max = Options::opts.k;
  size_t k_min = 16;
  size_t sample = gen(alg); 
  double mean_k = sample;
  double var_k = 0;

  double cumulative_sum = sample;
  double cumulative_quad_sum = sample * sample;

  double rho_k = 0;

  do {
    k++;
    sample = gen(alg); 
    cumulative_sum += sample;
    cumulative_quad_sum += (sample * sample);
    mean_k = cumulative_sum / ((double)k);
    var_k = ( cumulative_quad_sum - k*(mean_k*mean_k)  ) / ((double)(k-1));
    rho_k = std::sqrt( var_k / ((double)k));
  } while( k < k_min || (k < k_max
			 && ( std::abs(mean_k - e_model)
			      < ( rho_k * z_delta / precision ) ) ) );
   

  SampleEstimates est;
  est.sampleSize = k;
  est.sampleMean = mean_k;
  est.sampleVariance = var_k;

  return est;
}



// ----------------------------------------------------------------------
//                        ALGORITHMS COMPARISON
// ----------------------------------------------------------------------

class AlgorithmComparisonResult {
public:
  
  ~AlgorithmComparisonResult() {
    if (exactAlg) {
      delete exactAlg;
    }    
    if (s1) {
      delete s1;      
    }
    if (s2) {
      delete s2;
    }
  }

  void addExact(const EditDistanceInfo& info) {
    if (this->exactAlg) {
      delete this->exactAlg;
    }
    this->exactAlg = new EditDistanceInfo(info);
  }

  void addBandApprox(const EditDistanceInfo& info, size_t T) {
    this->bandApproxAlg[T] = info;
  }

  bool hasExact() {
    return (this->exactAlg != nullptr);
  }

  bool hasBandApproxWithT(size_t T) {
    return (this->bandApproxAlg.count(T));
  }

  EditDistanceInfo getExact() {
    return *(this->exactAlg);
  }

  EditDistanceInfo getBandApproxWithT(size_t T) {
    return this->bandApproxAlg[T];
  }

private:
  EditDistanceInfo* exactAlg = nullptr;
  std::map<size_t, EditDistanceInfo> bandApproxAlg;

  // if needed we may want to return also the string used to calculate
  // to compute the edit distances. Pointersa re used to minimize the
  // required memory (destructor will take care of freeing memory).
  std::string* s1 = nullptr;
  std::string* s2 = nullptr;

};


// --------------------------------------------------------------------
// NAMESPACES BEGIN

namespace lbio { namespace sim { namespace edit {


// returns the edit distance between strings encoded in two bits form
// on the 64 for bits input integers (strings can't be longer than 32
// characters). The actual lengths of the strings are given as
// parameters
lbio_size_t
edit_distance_encoded(uint64_t s1, lbio_size_t n1, uint64_t s2,
		    lbio_size_t n2, lbio_size_t** dpMatrix) {
  for (lbio_size_t i = 1; i < n1+1; ++i) {
    for(lbio_size_t j = 1; j < n2+1; ++j) {
      // pre compute matrix {A,C,G,T} x [1...n]
      uint64_t x = ( s1 >> 2*(i-1) ) & 0x3; 
      uint64_t y = ( s2 >> 2*(j-1) ) & 0x3;
      lbio_size_t delta = (x == y) ? 0 : 1;       
      dpMatrix[i][j] =
	std::min( std::min(dpMatrix[i-1][j]+1, dpMatrix[i][j-1]+1),
		  dpMatrix[i-1][j-1] + delta ) ;
    }
  }
  return dpMatrix[n1][n2];
}

void
edit_distance_exhastive_with_info(lbio_size_t n) {
  lbio_size_t** dpMatrix = allocMatrix<lbio_size_t>(n+1,n+1);
  // initialization of first row and column
  for (lbio_size_t i = 0; i < n+1; ++i) {
    dpMatrix[i][0] = i;
  }
  for (lbio_size_t j = 0; j < n+1; ++j) {
    dpMatrix[0][j] = j;
  }

  uint64_t N = pow(4,n);
  double tot_dist = 0;
  
  double min_dist = n;
  uint64_t min_center = 0;
  double max_dist = 0;
  uint64_t max_center = 0;
  
  for (uint64_t i = 0; i < N; ++i) {
    lbio_size_t dist = 0;
    
    for (uint64_t j = 0; j <N; ++j) {
      dist += edit_distance_encoded(i, n, j, n, dpMatrix);
    }

    double ddist = dist / static_cast<double>(N);
    tot_dist += dist;
    if (ddist < min_dist) {
      min_center = i;
      min_dist = ddist;      
    }
    if (ddist > max_dist) {
      max_center = i;
      max_dist = ddist;
    }
  }
  tot_dist /= static_cast<double>(N*N);
  freeMatrix(n+1, n+1, dpMatrix);
  std::cout << "Min: " << min_center << "\t" << min_dist << "\n";
  std::cout << "Max: " << max_center << "\t" << max_dist << "\n";
  std::cout << "Avg: " << tot_dist << "\n";
}

double
test_exhaustive_edit_distance_encoded(lbio_size_t n, double* freq) {

  lbio_size_t** dpMatrix = allocMatrix<lbio_size_t>(n+1,n+1);

  // initialization of first row and column
  for (lbio_size_t i = 0; i < n+1; ++i) {
    dpMatrix[i][0] = i;
  }
  for (lbio_size_t j = 0; j < n+1; ++j) {
    dpMatrix[0][j] = j;
  }

  uint64_t N = pow(4,n);
  double ed = 0;
  lbio_size_t dist = 0;
  for (uint64_t i = 0; i < N; ++i) {
    freq[0]++;
    for (uint64_t j = i+1; j <N; ++j) {
      dist = edit_distance_encoded(i, n, j, n, dpMatrix);
      freq[dist] += 2.0;
      ed += 2*dist;
    }
  }
  freeMatrix(n+1, n+1, dpMatrix);
  return ((double)ed) / ((double) (N*N));
}

double
exhaustive_edit_distance_improved(lbio_size_t n, std::vector<lbio_size_t>& freqs, lbio_size_t sigma) {

  // DP matrix initialization
  lbio_size_t** dpMatrix = allocMatrix<lbio_size_t>(n+1,n+1);  
  for (lbio_size_t i = 0; i < n+1; ++i) {
    dpMatrix[i][0] = i;
  }
  for (lbio_size_t j = 0; j < n+1; ++j) {
    dpMatrix[0][j] = j;
  }

  // cumulative counter to be returned
  double expected_ed = 0;
    
  // Generate all the x representatives of permutations of the bases
  // This is done generating all strings of the form
  // 0*1{0,1}*2{0,1,2}* ...
  
  // First decide how many distinct bases appear in the string
  for (lbio_size_t s = 1; s <= sigma; ++s) {    
    // Second we partition the integer n-s into s integers >=0.  This
    // represents the way blocks are sized in the n substrings. The
    // substring j for j=0,...,s-1 can be any string in {0,...,j}^nj
    // The generation of the partition is done by traversing a tree with 
    ListOfPartitions parts = recursive_int_partition(n, s);
    
  }
  return expected_ed;
}


// Useful alias used throughout the code
using ExactAlg    = EditDistanceWF<lbio_size_t, std::string>;
using BandApprAlg = EditDistanceBandApproxLinSpace<lbio_size_t, std::string>;


void
compare_edit_distance_algorithms(lbio_size_t n, lbio_size_t m,
				 lbio_size_t k, std::ostream& os) {
  lbio_size_t T_max = n / 2;
  lbio_size_t T_min = 1;

  ExactAlg exactAlg { n, m, {1,1,1} };

  GeometricProgression<lbio_size_t> geom(2, T_min);
  std::vector<lbio_size_t> Ts = geom.valuesLeq(T_max);
  Ts.push_back(0);
  

  
  lbio_size_t** dpMatrix = allocMatrix<lbio_size_t>(n+1, m+1); // !!!

  
  std::vector< std::shared_ptr<AlgorithmComparisonResult> > results;
  std::string s1(n, 'N');
  std::string s2(m, 'N');
  for (size_t l = 0; l < k; ++l) {
    std::shared_ptr<AlgorithmComparisonResult> res =
      std::make_shared<AlgorithmComparisonResult>();
    generateIIDString(s1);
    generateIIDString(s2);

    EditDistanceInfo tmp {};
    
    exactAlg.calculate(s1, s2);
    res->addExact(exactAlg.backtrack());

    // Approximation for all values of T
    for (auto T : Ts) {
      editDistanceBandwiseApproxMat(s1, s2, T, dpMatrix);
      closest_to_diagonal_backtrack(s1.size(), s2.size(), dpMatrix, tmp);
      res->addBandApprox(tmp, T);
    }
    results.push_back(res);
  }

  os << n << "\t";
  for (auto T : Ts) {
    os << T << "\t";
  }
  os << std::endl;
  for (auto pRes : results) {
    os << pRes->getExact() << "\t";
    for (auto T : Ts) {
      os << pRes->getBandApproxWithT(T) << "\t";
    }
    os << std::endl;
  }
  
  freeMatrix<lbio_size_t>(n+1, m+1, dpMatrix);
}


void
closest_to_diagonal_backtrack(size_t n, size_t m, size_t** dpMatrix,
			   EditDistanceInfo& info) {
  info.n_sub = 0;
  info.n_del = 0;
  info.n_ins = 0;
  info.edit_script = "";

  size_t i = n;
  size_t j = m;
  size_t a = 0, b = 0, c = 0, d = 0;
  // backtrack until one edge is reached
  while(i > 0 && j > 0) {
    a = dpMatrix[i-1][j-1];
    b = dpMatrix[i-1][j];
    c = dpMatrix[i][j-1];
    d = dpMatrix[i][j];
    
    // Match
    if ( (a == d) && ( a <= b) && (a <= c)) {
      info.edit_script = "M" + info.edit_script;
      i--; j--;
      continue;
    }
    
    // Substitution
    if ( (a < b) && (a < c) ) {
      info.n_sub++;
      info.edit_script = "S" + info.edit_script;
      i--; j--;
      continue;
    }
    // Deletion
    if ( (b < a) && (b < c) ) {
      info.n_del++;
      info.edit_script = "D" + info.edit_script;
      i--;
      continue;
    }
    // Insertion
    if ( (c < a) && (c < b) ) {
      info.n_ins++;
      info.edit_script = "I" + info.edit_script;
      j--;
      continue;
    }
    // Tie between all operations
    if ( (a == b) && (b == c) ) {
      if (i < j) {
	info.n_ins++;
	info.edit_script = "I" + info.edit_script;
	j--;
	continue;
      }
      if (i == j) {
	info.n_sub++;
	info.edit_script = "S" + info.edit_script;
	i--; j--;
	continue;
      }
      if (i > j) {
	info.n_del++;
	info.edit_script = "D" + info.edit_script;
	i--;
	continue;
      }
    }

    // Tie between sub and del
    if (a == b) {

      if (i > j) {
	info.n_del++;
	info.edit_script = "D" + info.edit_script;
	i--;
      } else {
	info.n_sub++;
	info.edit_script = "S" + info.edit_script;
	i--; j--;
      }
      continue;	
    }
    // Tie between sub and ins
    if (a == c) {
      if (i < j) {
	info.n_ins++;
	info.edit_script = "I" + info.edit_script;
	j--;
      } else {
	info.n_sub++;
	info.edit_script = "S" + info.edit_script;
	i--; j--;
      }
      continue;
    }

    // Tie between del and ins
    if (b == c) {
      if (j <= i) {
	info.n_del++;
	info.edit_script = "D" + info.edit_script;
	i--;
      } else {
	info.n_ins++;
	info.edit_script = "I" + info.edit_script;	
	j--;
      }
      continue;

    }
  }

  // backtrack the left edge (if needed)
  while(i > 0) {
    info.n_del++;
    info.edit_script = "D" + info.edit_script;
    i--;
  }

  // bacltrack the top edge (if needed)
  while(j > 0) {
    info.n_ins++;
    info.edit_script  = "I" + info.edit_script;
    j--;
  }
  

}

      
lbio_size_t
optimal_bandwidth_exact(lbio_size_t n, double precision, lbio_size_t Tmin) {
  lbio_size_t T = Tmin;
  lbio_size_t T_2 = static_cast<lbio_size_t>(std::floor(n / 2));
  BandApprAlg exactAlg {n, n, T_2 ,{1,1,1}};
  IidPairGenerator gen(n, n);
  lbio_size_t k_min = 5;
  while (T < n / 2) {
    double avg = 0;

    BandApprAlg apprAlg { n, n, T, {1,1,1} };
    for (lbio_size_t k = 0; k < k_min; ++k) {
      auto strings = gen();

      lbio_size_t exact = exactAlg.calculate(strings.first, strings.second);
      lbio_size_t approx = apprAlg.calculate(strings.first, strings.second);
      avg += (approx - exact) / static_cast<double>(exact);
    }
    if ( avg < k_min * precision) {
      return T;
    }
    T *= 2;
  }
  return T;
}

lbio_size_t
optimal_bandwidth(lbio_size_t n, double precision, lbio_size_t k, lbio_size_t Tmin) {
  lbio_size_t T_2 = Tmin;
  lbio_size_t T = 2 * T_2;
  
  BandApprAlg alg_T_2(n, n, T_2, {1,1,1});
  BandApprAlg alg_T(n, n, T, {1,1,1});
  IidPairGenerator gen(n,n);

  while (T < static_cast<lbio_size_t>(std::floor(n/2.0))) {
    double avg = 0;
    for (lbio_size_t i = 0; i < k; ++i) {
      auto strings = gen();
      lbio_size_t ed_T = alg_T.calculate(strings.first, strings.second);
      lbio_size_t ed_T_2 = alg_T_2.calculate(strings.first, strings.second);
      //     std::cout << (ed_T_2 - ed_T) << " ";
      avg += static_cast<double>(ed_T_2 - ed_T) / static_cast<double>(ed_T_2);
    }
    // std::cout << "\nT:    " << T
    // 	      << "\nAvg:  " << avg << "\n";

    avg /= static_cast<double>(k);
    if (avg < precision) {
      return T_2;
    }
    T_2 = T;
    T *= 2;
    
  }
    
  
  return T_2;
}

     
} } } // namespaces
