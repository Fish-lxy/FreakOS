#include "string.h"
#include "elf.h"
#include "types.h"
#include "vmm.h"

// 从 multiboot_t 结构获取ELF信息
ELF_t ELF_FromMultiBoot(MultiBoot_t* mb) {
	int i;
	ELF_t elf;
	ELF_SectionHeader_t* sh = (ELF_SectionHeader_t*) mb->addr;

	uint32_t shstrtab = sh[mb->shndx].addr;
	for (i = 0; i < mb->num; i++) {
		const char* name = (const char*) (shstrtab + sh[i].name + KERNEL_OFFSET);
		// 从 GRUB 提供的 multiboot 信息中寻找内核 ELF 格式所提取的字符串表和符号表
		if (strcmp(name, ".strtab") == 0) {
			elf.strtab = (const char*) sh[i].addr + KERNEL_OFFSET;
			elf.strtabsz = sh[i].size;
		}
		if (strcmp(name, ".symtab") == 0) {
			elf.symtab = (ELF_Symbol_t*) sh[i].addr + KERNEL_OFFSET;
			elf.symtabsz = sh[i].size;
		}
	}

	return elf;
}

// 查看ELF的符号信息
const char* ELF_SymbolLookup(uint32_t addr, ELF_t* elf) {
	addr += KERNEL_OFFSET;
	int i;

	for (i = 0; i < (elf->symtabsz / sizeof(ELF_Symbol_t)); i++) {
		if (ELF32_ST_TYPE(elf->symtab[i].info) != 0x2) {
			continue;
		}
		// 通过函数调用地址查到函数的名字(地址在该函数的代码段地址区间之内)
		if ((addr >= elf->symtab[i].value) && (addr < (elf->symtab[i].value + elf->symtab[i].size))) {
			return (const char*) ((uint32_t) elf->strtab + elf->symtab[i].name);
		}
	}

	return NULL;
}