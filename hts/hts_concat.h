void *malloc(size_t size);
void free(void *);

char *strncpy(char *dst, char *src, size_t size);
long int strtol (const char* str, char** endptr, int base);


struct hFILE;

//############################
//# kstring
//############################

typedef struct __kstring_t {
        size_t l, m;
                char *s;
} kstring_t;

static inline char *ks_release(kstring_t *s);
void kputsn(char *, int, kstring_t *);


//##########################
//# BGZF
//##########################
typedef struct __bgzidx_t bgzidx_t;
typedef struct z_stream_s z_stream;
struct BGZF;
typedef struct BGZF BGZF;
BGZF* bgzf_open(const char* path, const char *mode);
int bgzf_close(BGZF *fp);
BGZF* bgzf_hopen(struct hFILE *fp, const char *mode);
int bgzf_flush(BGZF *fp);

enum htsFormatCategory {
    unknown_category,
    sequence_data,    // Sequence data -- SAM, BAM, CRAM, etc
    variant_data,     // Variant calling data -- VCF, BCF, etc
    index_file,       // Index file associated with some data file
    region_list,      // Coordinate intervals or regions -- BED, etc
    category_maximum = 32767
};

enum htsExactFormat {
    unknown_format,
    binary_format, text_format,
    sam, bam, bai, cram, crai, vcf, bcf, csi, gzi, tbi, bed,
    json,
    format_maximum = 32767
};

enum htsCompression {
    no_compression, gzip, bgzf, custom,
    compression_maximum = 32767
};

typedef struct htsFormat {
    enum htsFormatCategory category;
    enum htsExactFormat format;
    struct { short major, minor; } version;
    enum htsCompression compression;
    short compression_level;  // currently unused
    void *specific;  // format specific options; see struct hts_opt.
} htsFormat;


//###########################
//# hts
//###########################
struct __hts_idx_t;
typedef struct __hts_idx_t hts_idx_t;
typedef struct {
    uint32_t is_bin:1, is_write:1, is_be:1, is_cram:1, is_bgzf:1, dummy:27;
    int64_t lineno;
    kstring_t line;
    char *fn, *fn_aux;
    union {
        BGZF *bgzf;
        struct cram_fd *cram;
        struct hFILE *hfile;
    } fp;
    htsFormat format;
} htsFile;


htsFile *hts_open(const char *fn, const char *mode);
int hts_close(htsFile *fp);

int hts_getline(htsFile *fp, int delimiter, kstring_t *str);
char **hts_readlines(const char *fn, int *_n);
int hts_set_threads(htsFile *fp, int n);
int hts_set_fai_filename(htsFile *fp, const char *fn_aux);

typedef int hts_readrec_func(BGZF *fp, void *data, void *r, int *tid, int *beg, int *end);
typedef const char *(*hts_id2name_f)(void*, int);



struct _h;
typedef struct _h hts_itr_t;

hts_idx_t *hts_idx_init(int n, int fmt, uint64_t offset0, int min_shift, int n_lvls);

void hts_idx_destroy(hts_idx_t *idx);
int hts_idx_push(hts_idx_t *idx, int tid, int beg, int end, uint64_t offset, int is_mapped);
void hts_idx_finish(hts_idx_t *idx, uint64_t final_offset);

void hts_idx_save(const hts_idx_t *idx, const char *fn, int fmt);
hts_idx_t *hts_idx_load(const char *fn, int fmt);

uint8_t *hts_idx_get_meta(hts_idx_t *idx, int *l_meta);
void hts_idx_set_meta(hts_idx_t *idx, int l_meta, uint8_t *meta, int is_copy);

int hts_idx_get_stat(const hts_idx_t* idx, int tid, uint64_t* mapped, uint64_t* unmapped);
uint64_t hts_idx_get_n_no_coor(const hts_idx_t* idx);

const char *hts_parse_reg(const char *s, int *beg, int *end);
hts_itr_t *hts_itr_query(const hts_idx_t *idx, int tid, int beg, int end, hts_readrec_func *readrec);
void hts_itr_destroy(hts_itr_t *iter);

int hts_itr_next(BGZF *fp, hts_itr_t *iter, void *r, void *data);
const char **hts_idx_seqnames(const hts_idx_t *idx, int *n, hts_id2name_f getid, void *hdr);


//###########################
//# tbx
//###########################
typedef struct {
        int32_t preset;
        int32_t sc, bc, ec; // seq col., beg col. and end col.
        int32_t meta_char, line_skip;
} tbx_conf_t;

typedef struct {
        tbx_conf_t conf;
        hts_idx_t *idx;
        void *dict;
} tbx_t;


int tbx_index_build(const char *fn, int min_shift, const tbx_conf_t *conf);
tbx_t *tbx_index_load(const char *fn);
const char **tbx_seqnames(tbx_t *tbx, int *n);  // free the array but not the values
void tbx_destroy(tbx_t *tbx);


hts_itr_t * tbx_itr_querys(tbx_t *tbx, char *);

int tbx_itr_next(htsFile *fp, tbx_t *tbx, hts_itr_t *iter, void *data);

//#####################################
//# sam.h
//#####################################

typedef htsFile samFile;

typedef struct {
        int32_t n_targets, ignore_sam_err;
        uint32_t l_text;
        uint32_t *target_len;
        int8_t *cigar_tab;
        char **target_name;
        char *text;
        void *sdict;
} bam_hdr_t;


typedef struct {
    int32_t tid;
    int32_t pos;
    uint16_t bin;
    uint8_t qual;
    uint8_t l_qname;
    uint16_t flag;
    uint8_t unused1;
    uint8_t l_extranul;
    uint32_t n_cigar;
    int32_t l_qseq;
    int32_t mtid;
    int32_t mpos;
    int32_t isize;
} bam1_core_t;

typedef struct {
        bam1_core_t core;
        int l_data, m_data;
        uint8_t *data;
        uint64_t id;
} bam1_t;



bam_hdr_t *sam_hdr_parse(int l_text, const char *text);
bam_hdr_t *sam_hdr_read(samFile *fp);
bam_hdr_t* bam_hdr_dup(const bam_hdr_t *h0);
int bam_hdr_write(BGZF *fp, const bam_hdr_t *h);
int sam_hdr_write(htsFile *fp, const bam_hdr_t *h);
int sam_write1(htsFile *fp, const bam_hdr_t *h, const bam1_t *b);



int sam_format1(const bam_hdr_t *h, const bam1_t *b, kstring_t *str);
int sam_read1(samFile *fp, bam_hdr_t *h, bam1_t *b);
int bam_read1(BGZF *fp, bam1_t *b);

bam1_t *bam_init1();
void bam_destroy1(bam1_t *b);
int bam_is_rev(bam1_t *b);
int bam_is_mrev(bam1_t *b);
char *bam_get_qname(bam1_t *b);
uint32_t *bam_get_cigar(bam1_t *b);
uint8_t *bam_get_seq(bam1_t *b);

uint8_t bam_seqi(uint8_t *c, int i);

uint8_t *bam_get_qual(bam1_t *b);

uint8_t *bam_get_aux(bam1_t *b);
int bam_get_l_aux(bam1_t *b);
uint8_t *bam_aux_get(const bam1_t *b, const char tag[2]);
int32_t bam_aux2i(const uint8_t *s);
float bam_aux2f(const uint8_t *s);
char *bam_aux2Z(const uint8_t *s);

bam1_t *bam_copy1(bam1_t *bdst, const bam1_t *bsrc);
bam1_t *bam_dup1(const bam1_t *bsrc);

int bam_cigar2qlen(int n_cigar, const uint32_t *cigar);
int bam_cigar2rlen(int n_cigar, const uint32_t *cigar);
int32_t bam_endpos(const bam1_t *b);

int   bam_str2flag(const char *str);    /** returns negative value on error */
char *bam_flag2str(int flag);   /** The string must be freed by the user */

int sam_parse1(kstring_t *s, bam_hdr_t *h, bam1_t *b);



hts_idx_t * sam_index_load(samFile *in, char *); // load index
int bam_index_build(const char *fn, int min_shift);


hts_itr_t * sam_itr_querys(hts_idx_t*, bam_hdr_t *h, char * region);

//int tbx_itr_next(htsFile *fp, tbx_t *tbx, hts_itr_t *iter, void *data);
int sam_itr_next(htsFile *fp, hts_itr_t *iter, void *data);

static const int BAM_CMATCH      = 0;
static const int BAM_CINS        = 1;
static const int BAM_CDEL        = 2;
static const int BAM_CREF_SKIP   = 3;
static const int BAM_CSOFT_CLIP  = 4;
static const int BAM_CHARD_CLIP  = 5;
static const int BAM_CPAD        = 6;
static const int BAM_CEQUAL      = 7;
static const int BAM_CDIFF       = 8;
static const int BAM_CBACK       = 9;

uint32_t bam_cigar_op(uint32_t cigar);
char bam_cigar_opchr(uint32_t cigar);
uint32_t bam_cigar_oplen(uint32_t cigar);


typedef union {
    void *p;
    int64_t i;
    double f;
} bam_pileup_cd;


typedef struct {
    bam1_t *b;
    int32_t qpos;
    int indel, level;
    uint32_t is_del:1, is_head:1, is_tail:1, is_refskip:1, aux:28;
    bam_pileup_cd cd; // generic per-struct data, owned by caller.
} bam_pileup1_t;


typedef int (*bam_plp_auto_f)(void *data, bam1_t *b);

struct __bam_plp_t;
typedef struct __bam_plp_t *bam_plp_t;

struct __bam_mplp_t;
typedef struct __bam_mplp_t *bam_mplp_t;

bam_plp_t bam_plp_init(bam_plp_auto_f func, void *data);
void bam_plp_destroy(bam_plp_t iter);
int bam_plp_push(bam_plp_t iter, const bam1_t *b);
const bam_pileup1_t *bam_plp_next(bam_plp_t iter, int *_tid, int *_pos, int *_n_plp);
const bam_pileup1_t *bam_plp_auto(bam_plp_t iter, int *_tid, int *_pos, int *_n_plp);
void bam_plp_set_maxcnt(bam_plp_t iter, int maxcnt);
void bam_plp_reset(bam_plp_t iter);

bam_mplp_t bam_mplp_init(int n, bam_plp_auto_f func, void **data);

void bam_mplp_init_overlaps(bam_mplp_t iter);
void bam_mplp_destroy(bam_mplp_t iter);
void bam_mplp_set_maxcnt(bam_mplp_t iter, int maxcnt);
int bam_mplp_auto(bam_mplp_t iter, int *_tid, int *_pos, int *n_plp, const bam_pileup1_t **plp);


//##############################################
//# kfunc
//##############################################

double kt_fisher_exact(int n11, int n12, int n21, int n22, double *_left, double *_right, double *two);

//##############################################
//# faidx
//##############################################

struct __faidx_t;
typedef struct __faidx_t faidx_t;



void fai_destroy(faidx_t *fai);
int fai_build(const char *fn);

faidx_t *fai_load(const char *fn);
//  @param  len  Length of the region; -2 if seq not present, -1 general error
char *fai_fetch(const faidx_t *fai, const char *reg, int *len);
int faidx_nseq(const faidx_t *fai);
char *faidx_fetch_seq(const faidx_t *fai, const char *c_name, int p_beg_i, int p_end_i, int *len);
int faidx_has_seq(const faidx_t *fai, const char *seq);

//##############################################
//# vcf
//##############################################


/*
typedef struct {
    int32_t rid;  // CHROM
    int32_t pos;  // POS
    int32_t rlen; // length of REF
    float qual;   // QUAL
    uint32_t n_info:16, n_allele:16;
    uint32_t n_fmt:8, n_sample:24;
    kstring_t shared, indiv;
    bcf_dec_t d; // lazy evaluation: $d is not generated by bcf_read(), but by explicitly calling bcf_unpack()
    int max_unpack;         // Set to BCF_UN_STR, BCF_UN_FLT, or BCF_UN_INFO to boost performance of vcf_parse when some of the fields won't be needed
    int unpacked;           // remember what has been unpacked to allow calling bcf_unpack() repeatedly without redoing the work
    int unpack_size[3];     // the original block size of ID, REF+ALT and FILTER
    int errcode;    // one of BCF_ERR_* codes
} bcf1_t;
*/


typedef struct {
    int type, n;    // variant type and the number of bases affected, negative for deletions
} variant_t;

typedef struct {
    int type;       // One of the BCF_HL_* type
    char *key;      // The part before '=', i.e. FILTER/INFO/FORMAT/contig/fileformat etc.
    char *value;    // Set only for generic lines, NULL for FILTER/INFO, etc.
    int nkeys;              // Number of structured fields
    char **keys, **vals;    // The key=value pairs
} bcf_hrec_t;


typedef struct {
    int id;             // id: numeric tag id, the corresponding string is bcf_hdr_t::id[BCF_DT_ID][$id].key
    int n, size, type;  // n: number of values per-sample; size: number of bytes per-sample; type: one of BCF_BT_* types
    uint8_t *p;         // same as vptr and vptr_* in bcf_info_t below
    uint32_t p_len;
    uint32_t p_off:31, p_free:1;
} bcf_fmt_t;

typedef struct {
    int key;        // key: numeric tag id, the corresponding string is bcf_hdr_t::id[BCF_DT_ID][$key].key
    int type, len;  // type: one of BCF_BT_* types; len: vector length, 1 for scalars
    union {
        int32_t i; // integer value
        float f;   // float value
    } v1; // only set if $len==1; for easier access
    uint8_t *vptr;          // pointer to data array in bcf1_t->shared.s, excluding the size+type and tag id bytes
    uint32_t vptr_len;      // length of the vptr block or, when set, of the vptr_mod block, excluding offset
    uint32_t vptr_off:31,   // vptr offset, i.e., the size of the INFO key plus size+type bytes
            vptr_free:1;    // indicates that vptr-vptr_off must be freed; set only when modified and the new
                            //    data block is bigger than the original
} bcf_info_t;

typedef struct {
    uint32_t info[3];  // stores Number:20, var:4, Type:4, ColType:4 in info[0..2]
                       // for BCF_HL_FLT,INFO,FMT and contig length in info[0] for BCF_HL_CTG
    bcf_hrec_t *hrec[3];
    int id;
} bcf_idinfo_t;

typedef struct {
    const char *key;
    const bcf_idinfo_t *val;
} bcf_idpair_t;


typedef struct {
	int32_t n[3];
    bcf_idpair_t *id[3];
    void *dict[3];          // ID dictionary, contig dict and sample dict
    char **samples;
    bcf_hrec_t **hrec;
    int nhrec, dirty;
    int ntransl, *transl[2];    // for bcf_translate()
    int nsamples_ori;           // for bcf_hdr_set_samples()
    uint8_t *keep_samples;
    kstring_t mem;
    int32_t m[3];
} bcf_hdr_t;


typedef struct {
    int m_fmt, m_info, m_id, m_als, m_allele, m_flt; // allocated size (high-water mark); do not change
    int n_flt;  // Number of FILTER fields
    int *flt;   // FILTER keys in the dictionary
    char *id, *als;     // ID and REF+ALT block (\0-seperated)
    char **allele;      // allele[0] is the REF (allele[] pointers to the als block); all null terminated
    bcf_info_t *info;   // INFO
    bcf_fmt_t *fmt;     // FORMAT and individual sample
    variant_t *var;     // $var and $var_type set only when set_variant_types called
    int n_var, var_type;
    int shared_dirty;   // if set, shared.s must be recreated on BCF output
    int indiv_dirty;    // if set, indiv.s must be recreated on BCF output
} bcf_dec_t;

typedef struct {
    int32_t rid;  // CHROM
    int32_t pos;  // POS
    int32_t rlen; // length of REF
    float qual;   // QUAL
    uint32_t n_info:16, n_allele:16;
    uint32_t n_fmt:8, n_sample:24;
    kstring_t shared, indiv;
    bcf_dec_t d; // lazy evaluation: $d is not generated by bcf_read(), but by explicitly calling bcf_unpack()
    int max_unpack;         // Set to BCF_UN_STR, BCF_UN_FLT, or BCF_UN_INFO to boost performance of vcf_parse when some of the fields won't be needed
    int unpacked;           // remember what has been unpacked to allow calling bcf_unpack() repeatedly without redoing the work
    int unpack_size[3];     // the original block size of ID, REF+ALT and FILTER
    int errcode;    // one of BCF_ERR_* codes
} bcf1_t;


bcf1_t *bcf_init(void);



bcf_hdr_t *bcf_hdr_read(htsFile *fp);
int bcf_hdr_set_samples(bcf_hdr_t *hdr, const char *samples, int is_file);

int bcf_read(htsFile *fp, const bcf_hdr_t *h, bcf1_t *v);

static inline const char *bcf_hdr_id2name(const bcf_hdr_t *hdr, int rid);

int bcf_unpack(bcf1_t *b, int which);

static const int BCF_DT_SAMPLE = 2;

int bcf_get_genotypes(const bcf_hdr_t *hdr, bcf1_t *line, int **dst, int *ndst);
int bcf_get_format_values(const bcf_hdr_t *hdr, bcf1_t *line, const char *tag, void **dst, int *ndst, int type);
//typedef htsFile vcfFile;





//##############################################
//# hts_extra
//##############################################
//int bam_get_read_seq(bam1_t *b, kstring_t *str);
