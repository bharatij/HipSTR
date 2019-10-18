#ifndef SNP_BAM_PROCESSOR_H_
#define SNP_BAM_PROCESSOR_H_

#include <iostream>
#include <string>
#include <vector>

#include "bam_processor.h"
#include "bam_io.h"
#include "base_quality.h"
#include "error.h"
#include "haplotype_tracker.h"
#include "region.h"
#include "vcf_reader.h"

const std::string HAPLOTYPE_TAG = "HP";
const double FROM_HAP_LL        = -0.01;   // Log-likelihood read comes from a haplotype if it matches BAM HP tag
const double OTHER_HAP_LL       = -1000.0; // Log-likelihood read comes from a haplotype if it differs from BAM HP tag

class SNPBamProcessor : public BamProcessor {
private:
  VCF::VCFReader* phased_snp_vcf_;
  int32_t match_count_, mismatch_count_;

  // Used to enforce pedigree requirements on SNPs used for phasing
  HaplotypeTracker* haplotype_tracker_;
  std::vector<NuclearFamily> families_;

  // Timing statistics (in seconds)
  double total_snp_phase_info_time_;
  double locus_snp_phase_info_time_;

  // Ignore any SNPs that are less than this many bases upstream/downstream of the STR
  int SKIP_PADDING;

  // Process reads from BAM generated by 10X genomics
  // Requires HP tag, which indicates which haplotype reads came from
  void process_10x_reads(std::vector<BamAlnList>& paired_strs_by_rg,
			 std::vector<BamAlnList>& mate_pairs_by_rg,
			 std::vector<BamAlnList>& unpaired_strs_by_rg,
			 const std::vector<std::string>& rg_names, const RegionGroup& region_group, const std::string& chrom_seq);

  // Extract the haplotype for an alignment based on the HP tag
  int get_haplotype(BamAlignment& aln) const;

  void verify_vcf_chromosomes(const std::vector<std::string>& chroms);

  // Private unimplemented copy constructor and assignment operator to prevent operations
  SNPBamProcessor(const SNPBamProcessor& other);
  SNPBamProcessor& operator=(const SNPBamProcessor& other);

public:
 SNPBamProcessor(bool use_bam_rgs, bool remove_pcr_dups) : BamProcessor(use_bam_rgs, remove_pcr_dups){
    SKIP_PADDING     = 15;
    match_count_     = 0;
    mismatch_count_  = 0;
    total_snp_phase_info_time_  = 0;
    locus_snp_phase_info_time_  = -1;
    phased_snp_vcf_             = NULL;
    haplotype_tracker_          = NULL;
  }

  ~SNPBamProcessor(){
    if (phased_snp_vcf_ != NULL)
      delete phased_snp_vcf_;
    if (haplotype_tracker_ != NULL)
      delete haplotype_tracker_;
  }

  double total_snp_phase_info_time() const { return total_snp_phase_info_time_; }
  double locus_snp_phase_info_time() const { return locus_snp_phase_info_time_; }

  void process_reads(std::vector<BamAlnList>& paired_strs_by_rg,
		     std::vector<BamAlnList>& mate_pairs_by_rg,
		     std::vector<BamAlnList>& unpaired_strs_by_rg,
		     const std::vector<std::string>& rg_names, const RegionGroup& region_group, const std::string& chrom_seq);

  virtual void analyze_reads_and_phasing(std::vector<BamAlnList>& alignments,
					 std::vector< std::vector<double> >& log_p1s, 
					 std::vector< std::vector<double> >& log_p2s,
					 const std::vector<std::string>& rg_names,
					 const RegionGroup& region_group, const std::string& chrom_seq) = 0;

  void set_input_snp_vcf(const std::string& vcf_file){
    if (phased_snp_vcf_ != NULL)
      delete phased_snp_vcf_;
    phased_snp_vcf_ = new VCF::VCFReader(vcf_file);
  }

  void use_pedigree_to_filter_snps(const std::vector<NuclearFamily>& families, const std::string& snp_vcf_file){
    if (phased_snp_vcf_ == NULL)
      printErrorAndDie("Cannot enforce pedigree structure on SNPs if no SNP VCF has been specified");
    if (haplotype_tracker_ != NULL)
      delete haplotype_tracker_;

    VCF::VCFReader pedigree_vcf_reader(snp_vcf_file);

    // Keep only those families where all members are present in the VCF
    families_.clear();
    std::set<std::string> snp_samples(pedigree_vcf_reader.get_samples().begin(), pedigree_vcf_reader.get_samples().end());
    for (auto family_iter = families.begin(); family_iter != families.end(); family_iter++)
      if (!family_iter->is_missing_sample(snp_samples))
	families_.push_back(*family_iter);
    haplotype_tracker_ = new HaplotypeTracker(families_, snp_vcf_file, 500000);
  }

  void finish(){
    if (match_count_ + mismatch_count_ > 0)
      selective_logger() << "\nSNP matching statistics: " << match_count_ << "\t" << mismatch_count_ << "\n";
  }
};

#endif
