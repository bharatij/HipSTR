#include <assert.h>
#include <math.h>

#include <algorithm>
#include <iostream>
#include <vector>

#include "../error.h"
#include "../mathops.h"
#include "StutterAligner.h"

double align_no_artifact_reverse(int block_len,                const char*   block_seq,
				 int base_seq_len,             const char*   base_seq,
				 const double* base_log_wrong, const double* base_log_correct){
  double log_prob = 0.0;
  for (int i = 0; i < base_seq_len; i++)
    log_prob += (block_seq[-i] == base_seq[-i] ? base_log_correct[-i] : base_log_wrong[-i]);
  return log_prob;
}


double align_no_artifact_forward(int block_len,                const char*   block_seq,
				 int base_seq_len,             const char*   base_seq,
				 const double* base_log_wrong, const double* base_log_correct){
  double log_prob = 0.0;
  for (int i = 0; i < base_seq_len; i++)
    log_prob += (block_seq[i] == base_seq[i] ? base_log_correct[i] : base_log_wrong[i]);
  return log_prob;
}

double align_pcr_insertion_reverse(int block_len,                const char*   block_seq,
				   int base_seq_len,             const char*   base_seq,
				   const double* base_log_wrong, const double* base_log_correct,
				   int D, int period){
  assert(D > 0 && base_seq_len <= block_len+D);
  std::vector<double> log_probs; log_probs.reserve(block_len+1);
  double log_prior = log(1.0/(block_len+1));

  // Compute probability for i = 0
  double log_prob = log_prior;
  for (int j = 0; j < std::min(D, base_seq_len); j++)
    log_prob += base_log_correct[-j]; // Bases matched with insertion
  for (int j = D; j < base_seq_len; j++)
    log_prob += (block_seq[-j+D] == base_seq[-j] ? base_log_correct[-j] : base_log_wrong[-j]); // Bases matched with block characters
  log_probs.push_back(log_prob);

  // Compute for all other i's, reusing previous result to accelerate computation
  int i = 1;
  for (; i <= std::min(std::max(0, base_seq_len-D), block_len); i++){
    // Update term for bases matched to insertions
    log_prob -= base_log_correct[-i+1];
    log_prob += base_log_correct[-i-D+1];
    
    // Update term for bases matched to stutter block
    log_prob -= (block_seq[-i+1] == base_seq[-i-D+1] ? base_log_correct[-i-D+1] : base_log_wrong[-i-D+1]);
    log_prob += (block_seq[-i+1] == base_seq[-i+1]   ? base_log_correct[-i+1]   : base_log_wrong[-i+1]);

    // Store LL for configuration
    log_probs.push_back(log_prob);
  }

  // Process remaining configurations 
  if (base_seq_len < block_len+D){
    while (i <= base_seq_len && i <= block_len){
      log_prob -= base_log_correct[-i+1];
      log_prob += (block_seq[-i+1] == base_seq[-i+1] ? base_log_correct[-i+1] : base_log_wrong[-i+1]);
      log_probs.push_back(log_prob);
      i++;
    }

    // Remaining configurations all have same likelihood so count all of them
    if (i <= block_len)
      log_probs.push_back(log(block_len+1-i)+log_prob);
  }

  // Convert to raw probabilities, add, take the log while avoiding underflow
  return fast_log_sum_exp(log_probs);
}

double align_pcr_insertion_forward(int block_len,                const char*   block_seq,
				   int base_seq_len,             const char*   base_seq,
				   const double* base_log_wrong, const double* base_log_correct,
				   int D, int period){
  assert(D > 0 && base_seq_len <= block_len+D);
  std::vector<double> log_probs; log_probs.reserve(block_len+1);
  double log_prior = log(1.0/(block_len+1));

  // Compute probability for i = 0
  double log_prob = log_prior;
  for (int j = 0; j < std::min(D, base_seq_len); j++)
    log_prob += base_log_correct[j]; // Bases matched with insertion
  for (int j = D; j < base_seq_len; j++)
    log_prob += (block_seq[j-D] == base_seq[j] ? base_log_correct[j] : base_log_wrong[j]); // Bases matched with block characters
  log_probs.push_back(log_prob);

  // Compute for all other i's, reusing previous result to accelerate computation
  int i = 1;
  for (; i <= std::min(std::max(0, base_seq_len-D), block_len); i++){
    // Update term for bases matched to insertions
    log_prob -= base_log_correct[i-1];
    log_prob += base_log_correct[i+D-1];
    
    // Update term for bases matched to stutter block
    log_prob -= (block_seq[i-1] == base_seq[i+D-1] ? base_log_correct[i+D-1] : base_log_wrong[i+D-1]);
    log_prob += (block_seq[i-1] == base_seq[i-1]   ? base_log_correct[i-1]   : base_log_wrong[i-1]);

    // Store LL for configuration
    log_probs.push_back(log_prob);
  }

  // Process remaining configurations 
  if(base_seq_len < block_len+D){
    while (i <= base_seq_len && i <= block_len){
      log_prob -= base_log_correct[i-1];
      log_prob += (block_seq[i-1] == base_seq[i-1] ? base_log_correct[i-1] : base_log_wrong[i-1]);
      log_probs.push_back(log_prob);
      i++;
    }

    // Remaining configurations all have same likelihood so count all of them
    if (i <= block_len)
      log_probs.push_back(log(block_len+1-i)+log_prob);
  }

  // Convert to raw probabilities, add, take the log while avoiding underflow
  return fast_log_sum_exp(log_probs);
}

double align_pcr_deletion_reverse(int block_len,                const char*   block_seq,
				  int base_seq_len,             const char*   base_seq,
				  const double* base_log_wrong, const double* base_log_correct,
				  int D){
  assert(D < 0 && block_len+D >= 0 && base_seq_len <= block_len+D);
  std::vector<double> log_probs; log_probs.reserve(block_len+D+1);
  double log_prior = log(1.0/(block_len+D+1));

  // Compute probability for i = 0
  double log_prob = log_prior;
  for (int j = 0; j < base_seq_len; j++)
    log_prob += (block_seq[-j+D] == base_seq[-j] ? base_log_correct[-j] : base_log_wrong[-j]);
  log_probs.push_back(log_prob);

  // Compute for all other i's, reusing previous result to accelerate computation
  for (int i = 1; i <= base_seq_len; i++){
    log_prob    -= (block_seq[-i+1+D] == base_seq[-i+1] ? base_log_correct[-i+1] : base_log_wrong[-i+1]);
    log_prob    += (block_seq[-i+1]   == base_seq[-i+1] ? base_log_correct[-i+1] : base_log_wrong[-i+1]);
    log_probs.push_back(log_prob);
  }

  // Remaining configurations all have same likelihood so count all of them
  if (base_seq_len < block_len+D)
    log_probs.push_back(log(block_len+D-base_seq_len)+log_prob);

  // Convert to raw probabilities, add, take the log while avoiding underflow
  return fast_log_sum_exp(log_probs);
}

double align_pcr_deletion_forward(int block_len,                const char*   block_seq,
				  int base_seq_len,             const char*   base_seq,
				  const double* base_log_wrong, const double* base_log_correct,
				  int D){
  assert(D < 0 && block_len+D >= 0 && base_seq_len <= block_len+D);
  std::vector<double> log_probs; log_probs.reserve(block_len+D+1);
  double log_prior = log(1.0/(block_len+D+1));

  // Compute probability for i = 0
  double log_prob = log_prior;
  for (int j = 0; j < base_seq_len; j++)
    log_prob += (block_seq[j-D] == base_seq[j] ? base_log_correct[j] : base_log_wrong[j]);
  log_probs.push_back(log_prob);

  // Compute for all other i's, reusing previous result to accelerate computation
  for (int i = 1; i <= base_seq_len; i++){
    log_prob    -= (block_seq[i-1-D] == base_seq[i-1] ? base_log_correct[i-1] : base_log_wrong[i-1]);
    log_prob    += (block_seq[i-1]   == base_seq[i-1] ? base_log_correct[i-1] : base_log_wrong[i-1]);
    log_probs.push_back(log_prob);
  }

  // Remaining configurations all have same likelihood so count all of them
  if (base_seq_len < block_len+D)
    log_probs.push_back(log(block_len+D-base_seq_len)+log_prob);

  // Convert to raw probabilities, add, take the log while avoiding underflow
  return fast_log_sum_exp(log_probs);
}



double align_stutter_region_reverse(int block_len,                const char*   block_seq,
				    int base_seq_len,             const char*   base_seq,
				    const double* base_log_wrong, const double* base_log_correct,
				    int D, int period){
  if (D == 0)
    return align_no_artifact_reverse(block_len, block_seq, base_seq_len, base_seq, base_log_wrong, base_log_correct);
  else if (D > 0)
    return align_pcr_insertion_reverse(block_len, block_seq, base_seq_len, base_seq, base_log_wrong, base_log_correct, D, period);
  else
    return align_pcr_deletion_reverse(block_len, block_seq, base_seq_len, base_seq, base_log_wrong, base_log_correct, D);
}

double align_stutter_region_forward(int block_len,                const char*   block_seq,
				    int base_seq_len,             const char*   base_seq,
				    const double* base_log_wrong, const double* base_log_correct,
				    int D, int period){
  if (D == 0)
    return align_no_artifact_forward(block_len, block_seq, base_seq_len, base_seq, base_log_wrong, base_log_correct);
  else if (D > 0)
    return align_pcr_insertion_forward(block_len, block_seq, base_seq_len, base_seq, base_log_wrong, base_log_correct, D, period);
  else
    return align_pcr_deletion_forward(block_len, block_seq, base_seq_len, base_seq, base_log_wrong, base_log_correct, D);
}
