#include "string.h"
#include "multiboot.h"
#include "elf.h"
#include "types.h"
#include "mm.h"

// 从 Multiboot_t 结构获取ELF信息
void ELF_FromMultiBoot(MultiBoot_t* mb, ELF_t* out) {
	int i;
	ELF_SectionHeader_t* sh = (ELF_SectionHeader_t*) mb->addr;

	uint32_t shstrtab = sh[mb->shndx].addr;
	for (i = 0; i < mb->num; i++) {
		const char* name = (const char*) (shstrtab + sh[i].name + KERNEL_OFFSET);
		// 从 GRUB 提供的 multiboot 信息中寻找内核 ELF 格式所提取的字符串表和符号表
		if (strcmp(name, ".strtab") == 0) {
			out->strtab = (const char*) sh[i].addr + KERNEL_OFFSET;
			out->strtabsz = sh[i].size;
		}
		if (strcmp(name, ".symtab") == 0) {
			if (out->symtab <= KERNEL_OFFSET) {
				out->symtab = (ELF_Symbol_t*) ((uint32_t) sh[i].addr + KERNEL_OFFSET);
				out->symtabsz = sh[i].size;
			}
			else {
				out->symtab = (ELF_Symbol_t*) sh[i].addr;
				out->symtabsz = sh[i].size;
			}

			out->symtabsz = sh[i].size;
		}
	}
}

// 查看ELF的符号信息
const char* ELF_SymbolLookup(uint32_t addr, ELF_t* elf) {
	ELF_Symbol_t* elfsymtab = elf->symtab;
	elfsymtab = (ELF_Symbol_t*) ((uint32_t) elfsymtab);
	const char* elfstrtab = elf->strtab;
	//elfstrtab = (const char*) ((uint32_t) elfsymtab);

	for (int i = 0; i < (elf->symtabsz / sizeof(ELF_Symbol_t)); i++) {
		if (ELF32_ST_TYPE(elfsymtab[i].info) != 0x2) {
			continue;
		}
		// 通过函数调用地址查到函数的名字(地址在该函数的代码段地址区间之内)
		if ((addr >= elfsymtab[i].value) && (addr < (elfsymtab[i].value + elfsymtab[i].size))) {
			return (const char*) ((uint32_t) elfstrtab + elfsymtab[i].name);
		}
	}
	return NULL;
}