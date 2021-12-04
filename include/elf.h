#ifndef ELF_H
#define ELF_H

#include "types.h"
#include "multiboot.h"

#define ELF32_ST_TYPE(i) ((i)&0xf)

// ELF 格式区段头
typedef
struct ELF_SectionHeader_t {
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
} __attribute__((packed)) ELF_SectionHeader_t;

// ELF 格式符号
typedef
struct ELF_Symbol_t {
  uint32_t name;
  uint32_t value;
  uint32_t size;
  uint8_t  info;
  uint8_t  other;
  uint16_t shndx;
} __attribute__((packed)) ELF_Symbol_t;

// ELF 信息
typedef
struct ELF_t {
  ELF_Symbol_t *symtab;
  uint32_t      symtabsz;
  const char   *strtab;
  uint32_t      strtabsz;
} ELF_t;

// 从 multiboot_t 结构获取ELF信息
ELF_t ELF_FromMultiBoot(MultiBoot_t *mb);

// 查看ELF的符号信息
const char *ELF_LookupSymbol(uint32_t addr, ELF_t *elf);

#endif 	// INCLUDE_ELF_H_