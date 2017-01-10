# HipSTR
**H**aplotype-based **p**hasing and genotyping of **STR**s  
![HipSTR icon!](https://raw.githubusercontent.com/tfwillems/HipSTR/master/img/HipSTR_icon_small.png)
#### Author: Thomas Willems <hipstrtool@gmail.com> <br> License: GNU v2

[Introduction](#introduction)  
[Installation](#installation)  
[Quick Start](#quick-start) 
[Tutorial](#tutorial)  
[In-depth Usage](#in-depth-usage)  
[Phasing](#phasing)     
[De novo Mutations](#De-novo-mutations)     
[Speed](#speed)  
[Call Filtering](#call-filtering)  
[Additional Usage Options](#additional-usage-options)  
[File Formats](#file-formats)   
[Citation](#citation)

## Introduction
Short tandem repeats [(STRs)](http://en.wikipedia.org/wiki/Microsatellite) are highly repetitive genomic sequences comprised of repeated copies of an underlying motif. Prevalent in most organisms' genomes, STRs are of particular interest because they mutate much more rapidly than most other genomic elements. As a result, they're extremely informative for genomic identification, ancestry inference and genealogy.

Despite their utility, STRs are particularly difficult to genotype. The repetitive sequence responsible for their high mutability also results in frequent alignment errors that can complicate and bias downstream analyses. In addition, PCR stutter errors often result in reads that contain additional or fewer repeat copies than the true underlying genotype. 

**HipSTR** was specifically developed to deal with these errors in the hopes of obtaining more robust STR genotypes. In particular, it accomplishes this by:

1. Learning locus-specific PCR stutter models using an [EM algorithm] (http://en.wikipedia.org/wiki/Expectation%E2%80%93maximization_algorithm)
2. Mining candidate STR alleles from population-scale sequencing data
3. Employing a specialized hidden Markov model to align reads to candidate sequences while accounting for STR artifacts
4. Utilizing phased SNP haplotypes to genotype and phase STRs


## Installation
HipSTR requires a standard c++ compiler and [CMake](http://www.cmake.org/download/).
To obtain HipSTR and all of its associated submodules, use:

    % git clone --recursive https://github.com/tfwillems/HipSTR.git

To build, use Make:

    % cd HipSTR
    % make

## Quick Start
To run HipSTR in its most broadly applicable mode, run it on **all samples concurrently** using the syntax:
```
./HipSTR --bams          run1.bam,run2.bam,run3.bam,run4.bam
         --fasta         /data/
         --regions       str_regions.bed
         --str-vcf       str_calls.vcf.gz
```
* **--bam** :  a comma-separated list of [BAM](#bams) files generated by [BWA-MEM](http://bio-bwa.sourceforge.net/bwa.shtml) and sorted and indexed using [samtools](http://www.htslib.org/)
* **--regions** : a [BED](#str-bed) file containing the coordinates for each STR region of interest. Download BED files for various organisms, including humans, from [here]( https://hipstr-tool.github.io/HipSTR-resources/) 
* **--fasta** : the directory containing [FASTA files] (https://en.wikipedia.org/wiki/FASTA_format) for each chromosome in the BED file. In the above usage example, if *str_regions.bed* contains chr1, chr2, and chr10, the corresponding files would be */data/chr1.fa*, */data/chr2.fa* and */data/chr10.fa*. Alternatively, you can supply the path for a single FASTA file containing all of the relevant sequences
* **--str-vcf** : The output path for the STR genotypes


For each region in *str_regions.bed*, **HipSTR** will:

1. Learn a stutter model for each locus
2. Use the stutter model and haplotype-based alignment algorithm to genotype each individual
3. Output the resulting STR genotypes to *str_calls.vcf.gz*, a [bgzipped] (http://www.htslib.org/doc/tabix.html) [VCF](#str-vcf) file. This VCF will contain calls for each sample in any of the BAM files' read groups. 

## Tutorial
To demonstrate how you can quickly apply HipSTR to whole-genome sequencing datasets, we've built a simple [tutorial](https://hipstr-tool.github.io/HipSTR-tutorial/). In less than 10 minutes, this tutorial will teach you how to genotype ~600 STRs in a deeply sequenced trio of individuals and inspect the results.

## In-depth Usage
**HipSTR** has a variety of usage options designed to accomodate scenarios in which the sequencing data varies in terms of the number of samples and the coverage. Most scenarios will fall into one of the following categories:

1. 200 or more low-coverage (~5x) samples
    * Sufficient reads for stutter estimation
    * Sufficient reads to detect candidate STR alleles
    * [**Use de novo stutter estimation + STR calling with de novo allele generation**](#mode-1)
2. 50 or more high-coverage (~30x) samples
    * Sufficient reads for stutter estimation
    * Sufficient reads to detect candidate STR alleles
    * [**Use de novo stutter estimation + STR calling with de novo allele generation**](#mode-1)
3. Handful of low-coverage  (~5x) samples
    * Insufficient reads for stutter estimation
    * Insufficient reads to detect candidate STR alleles
    * [**Use external stutter models + STR calling with a reference panel**](#mode-3)
4. Handful of high-coverage (~30x) samples
    * Insufficient samples for stutter estimation
    * Sufficient reads to detect candidate STR alleles
    * [**Use external stutter models + STR calling with de novo allele generation**](#mode-2)

<a id="mode-1"></a>
#### 1. De novo stutter estimation + STR calling with de novo allele generation
This mode is identical to the one suggested in the **Quick Start** section as it suits most applications. HipSTR will output the learned stutter models to *stutter_models.txt* and the STR genotypes in bgzipped VCF format to *str_calls.vcf.gz* 
```
./HipSTR --bams             run1.bam,run2.bam,run3.bam,run4.bam
         --fasta            /data/
         --regions          str_regions.bed
         --str-vcf          str_calls.vcf.gz
```

<a id="mode-2"></a>
#### 2. External stutter models + STR calling with de novo allele generation
The sole difference in this mode is that we no longer learn stutter models using the EM algorithm but instead input them from the **--stutter-in** file. For more details on the stutter model file format, see [below](#stutter-file).
```
./HipSTR --bams             run1.bam,run2.bam,run3.bam,run4.bam
         --fasta            /data/
         --regions          str_regions.bed
         --stutter-in       ext_stutter_models.txt
         --str-vcf          str_calls.vcf.gz
```

<a id="mode-3"></a>
#### 3. External stutter models + STR calling with a reference panel
This mode is very similar to mode #2, except that we provide an additional VCF file containing known STR genotypes at each locus using the **--str-vcf** option. **HipSTR** will not identify any additional candidate STR alleles in the BAMs when this option is specified, so it's best to use a VCF that contains STR genotypes for a wide range of populations and individuals. 
```
./HipSTR --bams             run1.bam,run2.bam,run3.bam,run4.bam
         --fasta            /data/
         --regions          str_regions.bed
         --stutter-in       ext_stutter_models.txt
         --ref-vcf          ref_strs.vcf.gz
         --str-vcf          str_calls.vcf.gz
```

### Phasing
HipSTR utilizes phased SNP haplotypes to phase the resulting STR genotypes. To do so, it looks for pairs of reads in which the STR-containing read or its mate pair overlap a samples's heterozygous SNP. In these instances, the quality score for the overlapping base can be used to determine the likelihood that the read came from each haplotype. Alternatively, when this information is not available, we assign the read an equal likelihood of coming from either strand. These likelihoods are incorporated into the HipSTR genotyping model which outputs phased genotypes. The quality of a phasing is reflected in the *PQ* FORMAT field, which provides the posterior probability of each sample's phased genotype. For homozygous genotypes, this value will always equal the *Q* FORMAT field as phasing is irrelevant. However, for heterozygous genotypes, if *PQ ~ Q*, it indicates that one of the two phasings is much more favorable. Alterneatively, if none of a sample's reads overlap heterozygous SNPs, both phasings will be equally probable and *PQ ~ Q/2*. To enable the use of physical phasing, supply HipSTR with the `--snp-vcf` option and a SNP VCF containing **phased** haplotypes. The schematic below outlines the concepts underlying HipSTR's physical phasing model:

![Phasing schematic!](https://raw.githubusercontent.com/tfwillems/HipSTR/master/img/phasing.png)

## De novo Mutations
To detect *de novo* STR mutations in families, one option is to generate HipSTR calls as described above, apply stringent filters to the genotypes (see below), and identify any sites where the genotypes are inconsistent with Mendelian inheritance. We used this approach in our [paper](http://biorxiv.org/content/early/2016/09/27/077727.full.pdf) to identify hundreds of replicable STR mutations in a well-studied trio in genomics. 

Although this approach can be effective, it does not provide a quantitative measure of the confidence in each identified mutation. To overcome this limitation, we've recently been developing **DenovoFinder**, a part of the *HipSTR* package that provides a more robust quantitative approach. This tool takes VCF files produced by **HipSTR** as input and computes the *likelihood* that a *de novo* mutation occurred at each STR for each family. To run **DenovoFinder**, your VCF file must contain a special type of field called phased genotype likelihoods (FORMAT field = PHASEDGL). As these are not included in the VCF by default, ensure that you run HipSTR with both the **--snp-vcf** and **--output-phased-gls** options if you're interested in using this module in later analyses.

We'll add more documentation and a tutorial regarding **DenovoFinder** in the coming weeks, but in the mean time, please type `./DenovoFinder --help` for more information. As **DenovoFinder** is unpublished work, we ask that you don't publish any manuscripts using it until we've written a short publication describing its methodology and applications. 

## Speed
HipSTR doesn't currently have multi-threaded support, but there are several options available to accelerate analyses:

1. Analyze each chromosome in parallel using the **--chrom** option. For example, **--chrom chr2** will only genotype the BED regions on chr2
2. Split your BED file into *N* files and analyze each of the *N* files in parallel. This allows you to parallelize analyses in a manner similar to option 1 but can be used for increased speed if *N* is much greater than the number of chromosomes.
3. If you have hundreds of BAM files, we recommend that you merge them into a more manageable number (10-100) using the `samtools merge` command. Large numbers of BAMs can lead to slow disk IO and poor performance

## Call Filtering
Although **HipSTR** mitigates many of the most common sources of STR genotyping errors, it's still extremely important to filter the resulting VCFs to discard low quality calls. To facilitate this process, the VCF output contains various FORMAT and INFO fields that are usually indicators of problematic calls. The INFO fields indicate the aggregate data for a locus and, if certain flags are raised, may suggest that the entire locus should be discarded. In contrast, FORMAT fields are available on a per-sample basis for each locus and, if certain flags are raised, suggest that some samples' genotypes should be discarded. The list below includes some of these fields and how they can be informative:

####INFO fields:  
1. **DP**: Reports the total depth/number of informative reads for all samples at the locus. The mean coverage per-sample can obtained by dividing this value by the number of samples with non-missing genotypes. In general, genotypes with a low mean coverage are unreliable because the reads may only have captured one of the two alleles if an individual is heterozygous.
2. **DSTUTTER**: Reports the total number of reads at a locus with what HipSTR thinks is a stutter artifact. If the total fraction of reads with stutter (DSTUTTER/DP) is high, genotypes for a locus will be unreliable because the reads frequently don't reflect the true underlying genotype. A high fraction of stutter-containing reads can be caused by too much PCR amplification, a duplicated locus that is mapping to a single location in the genome, or a failure of HipSTR to identify sufficient candidate alleles.  
3. **DFLANKINDEL**: Reports the total number of reads for which the maximum likelihood alignment contains an indel in the regions flanking the STR. A high fraction of reads with this artifact (DFLANKINDEL/DP) can be caused by an actual indel in a region neighboring the STR. However, it can also arise if HipSTR fails to identify sufficient candidate alleles. When these alleles are very different in size from the candidate alleles or are non-unit multiples, they're frequently aligned as indels in the flanking sequences.

####FORMAT fields:  
1. **Q**: Reports the posterior probability of the genotype. We've found that this is the best indicator of quality of an individual sample's genotype and nearly always utilize it to filter calls.   
2. **DP**, **DSTUTTER** and **DFLANKINDEL**: Identical to the INFO field case, these fields are also available for each sample and can be used in the same way to identify problematic individual calls.  

**So what thresholds do we suggest for each of these fields?** The answer really depends on the quality of the sequencing data, the ploidy of the chromosome and the downstream applications. However, we typically apply the following filters usings scripts we've provided in the **scripts** subdirectory of the HipSTR folder:
```
    python scripts/filter_vcf.py    --vcf                   diploid_hipstr_calls.vcf.gz
                                    --min-call-qual         0.9
                                    --max-call-flank-indel  0.15
                                    --max-call-stutter      0.15
    
    python scripts/filter_haploid_vcf.py    --vcf                   haploid_str_calls.vcf.gz
                                            --min-call-qual         0.9
                                            --max-call-flank-indel  0.15
                                            --max-call-stutter      0.15
```
The resulting VCF, which is printed to the standard output stream, will omit calls on a sample-by-sample basis in which any of the following conditions are met: i) the posterior < 90%, ii) more than 15% of reads have a flank indel or iii) more than 15% of reads have a stutter artifact. Calls for samples failing these criteria will be replaced with a "." missing symbol as per the VCF specification. For more filtering options, type either
```
    python scripts/filter_vcf.py -h
    python scripts/filter_haploid_vcf.py -h
```


## Additional Usage Options
| Option | Description |
| :------| :-----------|
| **--viz-out       <aln_viz.html.gz>**     | Output a bgzipped file containing Needleman-Wunsch alignments <br> for each locus. The resulting file can be readily visualized with [VizAln](#aln-viz) 
| **--haploid-chrs  <list_of_chroms>**      | Comma separated list of chromosomes to treat as haploid <br> By default, all chromosomes are treated as diploid
| **--no-rmdup**                            | Don't remove PCR duplicates. By default, they'll be removed
| **--snp-vcf    <phased_snps.vcf.gz>**     | Bgzipped VCF file containing phased SNP genotypes for samples <br> that are being genotyped. These SNPs will be used to physically <br> phase any STRs when a read or its mate pair overlaps a heterozygous site <br> **Always use this option if you have available phased SNP genotypes**
| **--bam-samps     <list_of_read_groups>** | Comma separated list of samples in same order as BAM files. <br> Assign each read the sample corresponding to its file. By default, <br> each read must have an RG tag and and the sample is determined from the SM field
| **--bam-libs      <list_of_read_groups>** | Comma separated list of libraries in same order as BAM files. <br> Assign each read the library corresponding to its file. By default, <br> each read must have an RG tag and and the library is determined from the LB field <br> NOTE: This option is required when --bam-samps has been specified

This list is comprised of the most useful and frequently used additional options, but is not all encompassing. For a complete list of options, please type either `./HipSTR` or `./HipSTR --help`

<a id="aln-viz"></a>
## Alignment Visualization
When deciphering and inspecting STR calls, it's extremely useful to visualize the supporting reads. HipSTR facilitates this through the **--viz-out** option, which writes a bgzipped file containing alignments for each call that can be readily visualized using the **VizAln** command included in HipSTR main directory. If you're interested in visualizing alignments, you first need to index the file using tabix. 
For example, if you ran HipSTR with the option `--viz-out aln.html.gz`, you should use the command
```
tabix -p bed aln.html.gz
```
to generate a [tabix] (http://www.htslib.org/doc/tabix.html) index for the file so that we can rapidly extract alignments for a locus of interest. This command only needs to be run once after the file has been generated. 

You could then visualize the calls for sample *NA12878* at locus *chr1 3784267* using the command
```
./VizAln aln.html.gz chr1 3784267 NA12878
```
This command will automatically open a rendering of the alignments in your browser and might look something like:
![Read more words!](https://raw.githubusercontent.com/HipSTR-Tool/HipSTR-tutorial/master/viz_NA12878.png)
The top bar represents the reference sequence and the red text indicates the name of the sample and its associated call at the locus. The remaining rows indicate the alignment for each read used in genotyping. In this particular example, 14 reads have an *8bp deletion* and 14 reads have a *4bp insertion*. HipSTR therefore genotypes this sample as *-8 | 4*

If we wanted to inspect all calls for the same locus, we could  use the command 
```
./VizAln aln.html.gz chr1 3784267
```
To facilitate rendering these images for publications, we've also created a similar script that converts
these alignments into a PDF. This script can only be applied to one sample at a time, but the image above
can be generated in a file alignments.pdf as follows:
```
./VizAlnPdf aln.html.gz chr1 3784267 NA12878 alignments 1
```
NOTE: Because the **--viz-out** file can become fairly large if you're genotyping thousands of loci or thousands of samples, in some scenarios it may be best to rerun HipSTR using this option on the subset of loci which you wish to visualize.

## File Formats
<a id="bams"></a>
### BAM files
HipSTR requires [BAM](https://samtools.github.io/hts-specs/SAMv1.pdf) files produced by any indel-sensitive aligner. These files must have been sorted by position using the `samtools sort` command and then indexed using `samtools index`. To associate a read with its sample of interest, HipSTR uses read group information in the BAM header lines. These *@*RG lines must contain an *ID* field, an *LB* field indicating the library and an *SM* field indicating the sample. For example, if a BAM contained the following header line 
> *@*RG     ID:RUN1 LB:ERR12345        SM:SAMPLE789

an alignment with the RG tag 
> RG:Z:RUN1

will be associated with sample *SAMPLE789* and library *ERR12345*. In this manner, HipSTR can analyze BAMs containing more than one sample and/or more than one library and can handle BAMs in which a single sample's reads are spread across multiple files. 

Alternatively, if your BAM files lack *RG* information, you can use the **--bam-samps** and **-bam-libs** flags to specify the sample and library associated with each BAM. In this setting, however, a BAM can only contain a single library and a single read group. For example, the command
```
./HipSTR --bams             run1.bam,run2.bam,run3.bam,run4.bam
         --fasta            /data/
         --regions          str_regions.bed
         --str-vcf          str_calls.vcf.gz
         --bam-samps        SAMPLE1,SAMPLE1,SAMPLE2,SAMPLE3
         --bam-libs         LIB1,LIB2,LIB3,LIB4
```
essentially tells HipSTR to associate all the reads in the first two BAMS with *SAMPLE1*, all the reads in the third file with *SAMPLE2* and all the reads in the last BAM with *SAMPLE3*.

<a id="str-bed"></a>
### STR region BED file
The BED file containing each STR region of interest is a tab-delimited file comprised of 5 required columns and one optional column: 

1. The name of the chromosome on which the STR is located
2. The start position of the STR on its chromosome
3. The end position of the STR on its chromosome
4. The motif length (i.e. the number of bases in the repeat unit)
5. The number of copies of the repeat unit in the reference allele

The 6th column is optional and contains the name of the STR locus, which will be written to the ID column in the VCF. 
Below is an example file which contains 5 STR loci 

**NOTE: The table header is for descriptive purposes. The BED file should not have a header**

CHROM | START | END | MOTIF_LEN | NUM_COPIES | NAME
---- | ---- | ---- | --- | --- | ---
chr1    | 13784267    | 13784306    | 4 | 10 | GATA27E01
chr1    | 18789523    | 18789555    | 3 | 11 | ATA008
chr2    | 32079410    | 32079469    | 4 | 15 | AGAT117
chr17    | 38994441    | 38994492    | 4 | 12 | GATA25A04
chr17    | 55299940    | 55299992    | 4 | 13 | AAT245

We've provided various *BED* files containing STR loci for different organisms, including humans, [here]( https://hipstr-tool.github.io/HipSTR-resources/)

For other model organisms, we recommend that you 

1. Use [Tandem Repeats Finder](https://tandem.bu.edu/trf/trf.html) or other tools to scan the reference genome for STRs
2. Reformat the resulting output to conform with the format outlined above

<a id="str-vcf"></a>
### VCF file
For more information on the VCF file format, please see the [VCF spec](http://samtools.github.io/hts-specs/VCFv4.2.pdf). For filtering and parsing VCFs, we recommend the fantastic python package [PyVCF](http://pyvcf.readthedocs.org/en/latest/)

#### INFO fields
*INFO* fields contains statistics about each genotyped locus in the VCF. The INFO fields reported by HipSTR primarily describe the learned/supplied stutter model for the locus and its reference coordinates and sequence characteristics.

FIELD | DESCRIPTION
----- | -----------
INFRAME_PGEOM  | Parameter for in-frame geometric step size distribution
INFRAME_UP     | Probability that stutter causes an in-frame increase in obs. STR size
INFRAME_DOWN   | Probability that stutter causes an in-frame decrease in obs. STR size
OUTFRAME_PGEOM | Parameter for out-of-frame geometric step size distribution
OUTFRAME_UP    | Probability that stutter causes an out-of-frame increase in obs. STR size
OUTFRAME_DOWN  | Probability that stutter causes an out-of-frame decrease in obs. STR size
BPDIFFS        | Base pair difference of each alternate allele from the reference allele
START          | Inclusive start coodinate for the repetitive portion of the reference allele
END            | Inclusive end coordinate for the repetitive portion of the reference allele
PERIOD         | Length of STR motif
AN             | Total number of alleles in called genotypes
REFAC          | Reference allele count
AC             | Alternate allele counts
NSKIP          | Number of samples not genotyped due to various issues
NFILT          | Number of samples that were originally genotyped but have since been filtered
DP             | Total number of reads used to genotype all samples
DSNP           | Total number of reads with SNP information
DSTUTTER       | Total number of reads with a stutter indel in the STR region
DFLANKINDEL    | Total number of reads with an indel in the regions flanking the STR

#### FORMAT fields
*FORMAT* fields contain information about the genotype for each sample at the locus. In addition to the most probable phased genotype (*GT*), HipSTR reports information about the posterior likelihood of this genotype (*PQ*) and its unphased analog (*Q*). 

FIELD     | DESCRIPTION
--------- | -----------
GT        | Genotype
GB        | Base pair differences of genotype from reference
Q         | Posterior probability of unphased genotype
PQ        | Posterior probability of phased genotype
DP        | Number of valid reads used for sample's genotype
DSNP      | Number of reads with SNP phasing information
PDP       | Fractional reads supporting each haploid genotype
GLDIFF    | Difference in likelihood between the reported and next best genotypes
DSNP      | Total number of reads with SNP information
PSNP      | Number of reads with SNPs supporting each haploid genotype
DSTUTTER  | Number of reads with a stutter indel in the STR region
DFLANKINDEL | Number of reads with an indel in the regions flanking the STR
AB        | log10 of the allele bias pvalue, where 0 is no bias and more negative values are increasingly biased. Not applicable to homozygous genotypes
DAB       | Number of reads used in the allele bias calculation
ALLREADS  | Base pair difference observed in each read's Needleman-Wunsch alignment
MALLREADS | Maximum likelihood bp diff in each read based on haplotype alignments
GL        | log-10 genotype likelihoods
PL        | Phred-scaled genotype likelihoods

<a id="stutter-file"></a>
### Stutter model
To model PCR stutter artifacts, we assume that there are three types of stutter artifacts:

1. **In-frame changes**: Change the size of the STR in the read by multiples of the repeat unit. For instance, if the repeat motif is AGAT, in-frame changes could lead to differences of -8, -4, 4, 8, and so on. 
2. **Out-of-frame changes**: Change the size of the STR by non-multiples of the repeat unit. For instance, if the repeat motif is AGAT, out-of-frame changes could lead to differences of -3, -2, -1, 1, 2, 3 and so on. 
3. **No stutter change**: The size of the STR in the read is the same as the size of the underlying STR. 


Stutter model files contain the information necessary to model each of these artifacts in a **tab-delimited BED-like** format with exactly 9 columns. An example of such a file is as follows:

CHROM  | START       | END      | IGEOM | IDOWN | IUP   | OGEOM | ODOWN | OUP
-----  | ----------- | -------- | ----  | ----  | ---   | ----  | ---   | ---
chr1   | 13784267    | 13784306 | 0.95  | 0.05  | 0.01  | 0.9   | 0.01  | 0.001
chr1   | 18789523    | 18789555 | 0.8   | 0.01  | 0.05  | 0.9   | 0.001 | 0.001
chr2   | 32079410    | 32079469 | 0.9   | 0.01  | 0.01  | 0.9   | 0.001 | 0.001
chr17  | 38994441    | 38994492 | 0.9   | 0.001 | 0.001 | 0.9   | 0.001 | 0.001 
chr17  | 55299940    | 55299992 | 0.95  | 0.01  | 0.01  | 0.9   | 0.001 | 0.001

**NOTE: The table header is for descriptive purposes. The stutter file should not have a header**


Each of the stutter parameters is defined as follows:

| VARIABLE | DESCRIPTION
| -------- | --------
| IDOWN    | Probability that in-frame changes decrease the size of the observed STR allele
| IUP      |  Probability that in-frame changes increase the size of the observed STR allele
| ODOWN    | Probability that out-of-frame changes decrease the size of the observed STR allele
| OUP      |  Probability that out-of-frame changes increase the size of the observed STR allele
| IGEOM    | Parameter governing geometric step size distribution for in-frame changes
| OGEOM    | Paramter  governing geometric step size distribution for out-of-frame changes

## FAQ
1. **Can I run HipSTR if my dataset only contains single-ended reads?**     
**Yes.** HipSTR is designed for paired-end reads and uses mate pair information to filter reads that are potentially aligned to an incorrect STR prior to genotyping. By default, HipSTR therefore removes all reads without mate pairs. However, if your dataset only contains single-ended reads, specify the **--use-unpaired** option to avoid performing this filtering.  
2. **Can I use HipSTR to analyze PCR-amplified reads?**     
**Yes.** As HipSTR was designed to analyze WGS data, HipSTR automatically identifies and filters out PCR duplicates prior to genotyping. When analyzing PCR-amplified reads, HipSTR will label most reads as PCR duplicates as they share exactly the same coordinates. To overcome this issue, specify the **--no-rmdup** option to disable duplicate removal when analyzing this type of data.
3. **Why are some of the STRs in my BED file not present in the output VCF?**    
HipSTR only genotypes a region if at least **--min-reads** and at most **--max-reads** overlap the STR. It then attempts to learn the stutter model (if appropriate), build haplotypes for the region and perform genotyping. If any of these stages is unsuccessful, it skips the STR and continues on to the next region. The **--log** file contains the failure reason for each failed region as well as an overall summary of why regions were skipped at the end of the log. 

## Citation
If you found HipSTR useful, we would appreciate it if you could [cite](http://biorxiv.org/content/early/2016/09/27/077727) our preprint describing HipSTR and its applications: **[Genome-wide profiling of heritable and de novo STR variations](http://biorxiv.org/content/early/2016/09/27/077727.full.pdf)**
