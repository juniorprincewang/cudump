#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <elf.h>
#include <limits.h>
#include "list.h"

#define CUDA_ARCH_SM_1X 0x50 /* sm_1x */
#define CUDA_ARCH_SM_2X 0xc0 /* sm_2x */
#define CUDA_ARCH_SM_3X 0xe0 /* sm_3x */

#define Elf_Ehdr Elf64_Ehdr
#define Elf_Shdr Elf64_Shdr
#define Elf_Phdr Elf64_Phdr
#define Elf_Sym	 Elf64_Sym

#define SH_TEXT ".text."
#define SH_INFO ".nv.info"
#define SH_INFO_FUNC ".nv.info."
#define SH_LOCAL ".nv.local."
#define SH_SHARED ".nv.shared."
#define SH_CONST ".nv.constant"
#define SH_REL ".rel.nv.constant"
#define SH_RELSPACE ".nv.constant14"
#define SH_GLOBAL ".nv.global"
#define SH_GLOBAL_INIT ".nv.global.init"
#define NV_GLOBAL   0x10

typedef struct section_entry_ {
	uint16_t type;
	uint16_t size;
} section_entry_t;

typedef struct const_entry {
	uint32_t sym_idx;
	uint16_t base;
	uint16_t size;
} const_entry_t;

typedef struct func_entry {
	uint32_t sym_idx;
	uint32_t local_size;
} func_entry_t;

typedef struct param_entry {
	uint32_t pad; /* always -1 */
	uint16_t idx;
	uint16_t offset;
	uint32_t size;
} param_entry_t;

typedef struct stack_entry {
	uint16_t size;			
	uint16_t unk16;
	uint32_t unk32;
} stack_entry_t;

typedef struct crs_stack_size_entry {
	uint32_t size;
} crs_stack_size_entry_t;

typedef struct symbol_entry {
	uint64_t offset; /* offset in relocation (c14) */
	uint32_t unk32;
	uint32_t sym_idx;
} symbol_entry_t;


#define NVIDIA_CONST_SEGMENT_MAX_COUNT 32

struct cuda_param {
	int idx;
	uint32_t offset;
	uint32_t size;
	uint32_t flags;
	struct cuda_param *next;
};

struct cuda_raw_func {
	char *name;
	void *code_buf;
	uint32_t code_size;
	struct {
		void *buf;
		uint32_t size;
	} cmem[NVIDIA_CONST_SEGMENT_MAX_COUNT]; /* local to functions. */
	uint32_t reg_count;
	uint32_t bar_count;
	uint32_t stack_depth;
	uint32_t stack_size;
	uint32_t shared_size;
	uint32_t param_base;
	uint32_t param_size;
	uint32_t param_count;
	struct cuda_param *param_data;
	uint32_t local_size;
	uint32_t local_size_neg;
};

struct cuda_const_symbol {
	int idx; /* cX[] index. */
	char *name;
	uint32_t offset; /* offset in cX[]. */
	uint32_t size; /* size of const value. */
	struct list_head list_entry; /* entry to symbol list. */
};

struct CUmod_st {
	FILE *fp;
	void *bin;
	uint64_t code_addr;
	uint32_t code_size;
	uint64_t sdata_addr;
	uint32_t sdata_size;
	struct {
		uint64_t addr;
		uint32_t size;
		uint32_t raw_size;
		void *buf;
	} cmem[NVIDIA_CONST_SEGMENT_MAX_COUNT]; /* global to functions. */
	uint32_t func_count;
	uint32_t symbol_count;
	struct list_head func_list;
	struct list_head symbol_list;
	int arch;
};

struct CUfunc_st {
	struct cuda_raw_func raw_func;
	struct list_head list_entry;
	struct CUmod_st *mod;
};