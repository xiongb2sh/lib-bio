#include "common.hpp"

#include <iostream>
#include <memory>
#include <cstring>

size_t**
allocDPMatrix(size_t n, size_t m) {
  size_t** dpMatrix = new size_t*[n+1];
  for (int i = 0; i <= n; ++i) {
    dpMatrix[i] = new size_t[m+1];
  }
  return dpMatrix;
}

void
freeDPMatrix(size_t** dpMatrix, size_t n, size_t m) {
  for (int i = 0; i <= n; ++i) {
    delete[] dpMatrix[i];
  }
  delete[] dpMatrix;
}

void
printDPMatrix(size_t** dpMatrix, size_t n, size_t m) {
  for (size_t i = 0; i <= n; ++i) {
    for (size_t j = 0; j <=m; ++j) {
      std::cout << dpMatrix[i][j] << '\t';
    }
    std::cout << std::endl;
  }
}

void editInfoCompute(EditDistanceInfo& info) {
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

void
editDistanceMat(const std::string& s1, const std::string& s2, size_t** dpMatrix) {
  size_t n = s1.size();
  size_t m = s2.size();

  // initialization of first row and column
  for (size_t i = 0; i < n+1; ++i) {
    dpMatrix[i][0] = i;
  }
  for (size_t j = 0; j < m+1; ++j) {
    dpMatrix[0][j] = j;
  }
  
  for (int i = 1; i < n+1; ++i) {
    for(int j = 1; j < m+1; ++j) {
      size_t delta = (s1[i-1] == s2[j-1]) ? 0 : 1;
      dpMatrix[i][j] = MIN( MIN(dpMatrix[i-1][j]+1, dpMatrix[i][j-1]+1) , dpMatrix[i-1][j-1] + delta ) ;
    }
  }

}

size_t
editDistance(const std::string& s1, const std::string& s2) {
  size_t n = s1.size();
  size_t m = s2.size();
  size_t** dpMatrix = new size_t*[n+1];
  for (size_t i = 0; i < n+1; ++i) {
    dpMatrix[i] = new size_t[m+1];
  }

  editDistanceMat(s1, s2, dpMatrix);
 
  size_t dist = dpMatrix[n][m];
  for (int i = 0; i < n+1; ++i) {
    delete[] dpMatrix[i];   
  }
  delete[] dpMatrix;
  return dist;
}


void
editDistanceEstimations(size_t n_min, size_t n_max, size_t n_step, size_t k_max) {
  
  std::cout << std::endl;
  for (size_t n = n_min; n <= n_max; n += n_step) {
    std::string s1(n,'N');
    std::string s2(n,'N');
    double AED = 0;
    for (size_t k = 1; k <= k_max; ++k) {
      generateIIDString(s1);
      generateIIDString(s2);
      AED += editDistance(s1,s2);
    }
    std::cout << n << "\t" << ( AED / k_max) << std::endl;
  }
  std::cout << std::endl;
}


std::unique_ptr<size_t[]>
editDistSamples(size_t n, size_t k_samples) {
  std::unique_ptr<size_t[]> v(new size_t[k_samples]);
  std::string s1(n,'N');
  std::string s2(n,'N');
  size_t* v0 = new size_t[n];
  size_t* v1 = new size_t[n];
  for (int k = 0; k < k_samples; ++k) {
    generateIIDString(s1);
    generateIIDString(s2);
    v[k] = editDistanceLinSpace(s1,s2,v0,v1);
  }
  delete[] v0;
  delete[] v1;
  return v;
}

std::unique_ptr<EditDistanceInfo[]>
editDistSamplesInfo(size_t n, size_t k_samples) {
  std::unique_ptr<EditDistanceInfo[]> infos(new EditDistanceInfo[k_samples]);
  std::string s1(n,'N');
  std::string s2(n,'N');

  size_t** dpMatrix = allocDPMatrix(n,n);

  for (size_t k = 0; k < k_samples; ++k) {
    generateIIDString(s1);
    generateIIDString(s2);
    editDistanceMat(s1, s2, dpMatrix);
    editDistanceBacktrack(dpMatrix, n, n, infos[k]);
    editInfoCompute(infos[k]);
  }
  
  freeDPMatrix(dpMatrix, n, n);
  
  return infos;
}

void
editDistanceBacktrack(size_t** dpMatrix, size_t n, size_t m, EditDistanceInfo& info) {
  info.edit_script = "";
  size_t i = n;
  size_t j = m;  
  while( i > 0 && j > 0) {
    if (dpMatrix[i-1][j-1] <= dpMatrix[i-1][j]) {
      
      if(dpMatrix[i-1][j-1] <= dpMatrix[i][j-1]) {

	if (dpMatrix[i-1][j-1] == dpMatrix[i][j]) { 
	  info.edit_script = "M" + info.edit_script;
	} else {
	  info.edit_script = "S" + info.edit_script;
	}	
	i--; j--;
	continue;
	
      } else {
	info.edit_script = "I" + info.edit_script;
	j--;
	continue;
      }
    }
    info.edit_script = "D" + info.edit_script;
    i--;
  }

  while (i > 0) {
    info.edit_script = "D" + info.edit_script;
    i--;
  }

  while(j > 0) {
    info.edit_script = "I" + info.edit_script;
    j--;
  }
}


void editDistanceWithInfo(const std::string& s1, const std::string& s2, EditDistanceInfo& info) {
  size_t n = s1.size();
  size_t m = s2.size();
  size_t** dpMatrix = new size_t*[n+1];
  for (int i = 0; i < n+1; ++i) {
    dpMatrix[i] = new size_t[m+1];
  }

  editDistanceMat(s1, s2, dpMatrix);
  editDistanceBacktrack(dpMatrix, n, m, info);
  editInfoCompute(info);

  for (int i = 0; i < n+1; ++i) {
    delete[] dpMatrix[i];
  }
  delete[] dpMatrix;
}
