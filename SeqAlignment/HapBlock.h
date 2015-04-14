#ifndef HAPBLOCK_H_
#define HAPBLOCK_H_

#include <stdint.h>
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <string>
#include <vector>

#include "error.h"
#include "RepeatInfo.h"

class HapBlock {
 private:
  std::string ref_seq_;
  std::vector<std::string> alt_seqs_;
  int32_t start_;  // Start of region (inclusive)
  int32_t end_;    // End   of region (not inclusive)
  int max_size_;

  std::vector<int*> l_homopolymer_lens;
  std::vector<int*> r_homopolymer_lens;

  void calc_homopolymer_lengths();

 public:
  HapBlock(int32_t start, int32_t end, std::string ref_seq) {
    start_    = start;
    end_      = end;
    ref_seq_  = ref_seq;
    max_size_ = (int)ref_seq.size();
    alt_seqs_ = std::vector<std::string>();
  }

  ~HapBlock() {
    for (int i = 0; i < l_homopolymer_lens.size(); i++)
      delete [] l_homopolymer_lens[i];
    for (int i = 0; i < r_homopolymer_lens.size(); i++)
      delete [] r_homopolymer_lens[i];
    l_homopolymer_lens.clear();
    r_homopolymer_lens.clear();
  }

  RepeatInfo* get_repeat_info() {
    return NULL;
  }

  int32_t start()   const { return start_; }
  int32_t end()     const { return end_;   }
  int num_options() const { return 1 + alt_seqs_.size(); }

  void add_alternate(std::string alt) {
    alt_seqs_.push_back(alt);
    max_size_ = std::max(max_size_, (int)alt.size());
  }

  void order_alternates_by_length();

  int max_size() const { return max_size_; }

  void print(std::ostream& out);

  int size(int index) const {
    if (index == 0)
      return ref_seq_.size();
    else if (index-1 < alt_seqs_.size())
      return alt_seqs_[index-1].size();
    else
      throw std::out_of_range("Index out of bounds in HapBlock.size()");
  }

  const std::string& get_seq(int index) {
    if (index == 0)
      return ref_seq_;
    else if (index-1 < alt_seqs_.size())
      return alt_seqs_[index-1];
    else
      throw std::out_of_range("Index out of bounds in HapBlock.get_seq()");
  }

  inline int left_homopolymer_len(int seq_index, int base_index) {
    return (seq_index == 0 ? l_homopolymer_lens[0][base_index] : l_homopolymer_lens[seq_index-1][base_index]);
  }

  inline int right_homopolymer_len(int seq_index, int base_index) {
    return (seq_index == 0 ? r_homopolymer_lens[0][base_index] : r_homopolymer_lens[seq_index-1][base_index]);
  }
};

#endif