#ifndef __ELF_H
#define __ELF_H

#include "types.h"
#include "multiboot.h"


#define ELF_MAGIC   0x464C457FU         // "\x7FELF" in little endian

/* file header */
typedef struct ELF_Header_t {
    uint32_t e_magic;     // must equal ELF_MAGIC
    uint8_t e_elf[12];
    uint16_t e_type;      // 1=relocatable, 2=executable, 3=shared object, 4=core image
    uint16_t e_machine;   // 3=x86, 4=68K, etc.
    uint32_t e_version;   // file version, always 1
    uint32_t e_entry;     // entry point if executable
    uint32_t e_phoff;     // file position of program header or 0
    uint32_t e_shoff;     // file position of section header or 0
    uint32_t e_flags;     // architecture-specific flags, usually 0
    uint16_t e_ehsize;    // size of this elf header
    uint16_t e_phentsize; // size of an entry in program header
    uint16_t e_phnum;     // number of entries in program header or 0
    uint16_t e_shentsize; // size of an entry in section header
    uint16_t e_shnum;     // number of entries in section header or 0
    uint16_t e_shstrndx;  // section number that contains section name strings
}ELF_Header_t;

/* program section header */
typedef struct ELF_SectionHeader_t {
    uint32_t p_type;   // loadable code or data, dynamic linking info,etc.
    uint32_t p_offset; // file offset of segment
    uint32_t p_va;     // virtual address to map segment
    uint32_t p_pa;     // physical address, not used
    uint32_t p_filesz; // size of segment in file
    uint32_t p_memsz;  // size of segment in memory (bigger if contains bss）
    uint32_t p_flags;  // read/write/execute bits
    uint32_t p_align;  // required alignment, invariably hardware page size
}ELF_SectionHeader_t;

/* values for Proghdr::p_type */
#define ELF_PT_LOAD                     1

/* flag bits for Proghdr::p_flags */
#define ELF_PF_X                        1
#define ELF_PF_W                        2
#define ELF_PF_R                        4




//---------------------------------------------------
#define ELF32_ST_TYPE(i) ((i)&0xf)

// ELF 格式区段头
typedef
struct DebugELF_SectionHeader_t {
  uint32_t name;
  uint32_t type;
  uint32_t flags;
  uint32_t addr;
  uint32_t offset;
  uint32_t size;
  uint32_t link;
  uint32_t info;
  uint32_t addralign;
  uint32_t entsize;
} __attribute__((packed)) DebugELF_SectionHeader_t;

// ELF 格式符号
typedef
struct DebugELF_Symbol_t {
  uint32_t name;
  uint32_t value;
  uint32_t size;
  uint8_t  info;
  uint8_t  other;
  uint16_t shndx;
} __attribute__((packed)) DebugELF_Symbol_t;

// ELF 信息
typedef
struct DebugELF_t {
  DebugELF_Symbol_t *symtab;
  uint32_t      symtabsz;
  const char   *strtab;
  uint32_t      strtabsz;
} DebugELF_t;

// 从 multiboot_t 结构获取ELF信息
void MultiBootGetELF(MultiBoot_t *mb, DebugELF_t* out);

// 查看ELF的符号信息
const char *ELF_LookupSymbol(uint32_t addr, DebugELF_t *elf);

#endif 	// INCLUDE_ELF_H_