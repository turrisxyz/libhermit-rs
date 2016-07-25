/*
 * Copyright (c) 2016, Stefan Lankes, RWTH Aachen University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the University nor the names of its contributors
 *      may be used to endorse or promote products derived from this
 *      software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <multiboot.h>
#include <elf.h>
#include <page.h>

#define HALT	asm volatile ("hlt")

/*
 * Note that linker symbols are not variables, they have no memory allocated for
 * maintaining a value, rather their address is their value.
 */
extern const void kernel_start;
extern const void kernel_end;
extern const void bss_start;
extern const void bss_end;

void main(void)
{
	size_t limit = 0;
	elf_header_t* header = NULL;

	// initialize .bss section
	memset((void*)&bss_start, 0x00, ((size_t) &bss_end - (size_t) &bss_start));

	koutput_init();
	kputs("HermitCore loader...\n");
	kprintf("Loader starts at %p and ends at %p\n", &kernel_start, &kernel_end);

	page_init();

	if (mb_info) {
		if (mb_info->flags & MULTIBOOT_INFO_MEM_MAP) {
			size_t end_addr, start_addr;
			multiboot_memory_map_t* mmap = (multiboot_memory_map_t*) ((size_t) mb_info->mmap_addr);
			multiboot_memory_map_t* mmap_end = (void*) ((size_t) mb_info->mmap_addr + mb_info->mmap_length);

			// mark available memory as free
			while (mmap < mmap_end) {
				if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
					/* set the available memory as "unused" */
					start_addr = mmap->addr;
					end_addr = start_addr + mmap->len;

					if (limit < end_addr)
						limit = end_addr;
			
					kprintf("Free region 0x%zx - 0x%zx\n", start_addr, end_addr);
				}
				mmap = (multiboot_memory_map_t*) ((size_t) mmap + sizeof(uint32_t) + mmap->size);
			}
		} else {
			goto failed;
		}

		if (mb_info->flags & MULTIBOOT_INFO_MODS) {
			if (!mb_info->mods_count) {
				kputs("Ups, we need at least one module!\n");
				goto failed;
			}

			// per default the first module is our HermitCore binary
			multiboot_module_t* mmodule = (multiboot_module_t*) ((size_t) mb_info->mods_addr);
			header = (elf_header_t*) ((size_t) mmodule[0].mod_start);
			kprintf("ELF file is located at %p\n", header);
		}
	} else {
		goto failed;
	}

	if (BUILTIN_EXPECT(!header, 0))
		goto failed;

	if (BUILTIN_EXPECT(header->ident.magic != ELF_MAGIC, 0))
		goto invalid;

	if (BUILTIN_EXPECT(header->type != ELF_ET_EXEC, 0))
		goto invalid;

	if (BUILTIN_EXPECT(header->machine != ELF_EM_X86_64, 0))
		goto invalid;

	if (BUILTIN_EXPECT(header->ident._class != ELF_CLASS_64, 0))
		goto invalid;

	if (BUILTIN_EXPECT(header->ident.data != ELF_DATA_2LSB, 0))
		goto invalid;

	if (header->ident.pad[0] != 0x42) {
		kprintf("ELF file doesn't contain a HermitCore application (OS/ABI 0x%x)\n", (uint32_t)header->ident.pad[0]);
		goto invalid;
	}

	for (int i=0; i<header->ph_entry_count; i++) {
		elf_program_header_t* prog_header;

		prog_header = (elf_program_header_t*) (header->ph_offset+i*header->ph_entry_size+(size_t)header);

		switch(prog_header->type)
		{
		case  ELF_PT_LOAD: {	// load program segment
				size_t viraddr = prog_header->virt_addr;
				size_t phyaddr = prog_header->offset + (size_t)header;
				const size_t displacement = 0x200000ULL - (phyaddr & 0x1FFFFFULL);

				uint32_t npages = (prog_header->file_size >> PAGE_BITS);
				if (prog_header->file_size & (PAGE_SIZE-1))
					npages++;

				kprintf("Map %u pages from physical start address 0x%zx linear to 0x%zx\n", npages + (displacement >> PAGE_BITS), phyaddr, viraddr);
				int ret = page_map(viraddr, phyaddr, npages + (displacement >> PAGE_BITS), PG_GLOBAL|PG_RW);
				if (ret)
					goto failed;

				phyaddr += displacement;
				*((uint64_t*) (viraddr + 0x08)) = phyaddr; // physical start address
				*((uint64_t*) (viraddr + 0x10)) = limit;   // physical limit
				*((uint32_t*) (viraddr + 0x24)) = 1; // number of used cpus
				*((uint32_t*) (viraddr + 0x30)) = 0; // apicid
				*((uint64_t*) (viraddr + 0x38)) = prog_header->file_size;
				*((uint32_t*) (viraddr + 0x60)) = 1; // numa nodes

				// move file to a 2 MB boundary
				for(size_t va = viraddr+(npages << PAGE_BITS)+displacement-sizeof(uint8_t); va >= viraddr+displacement; va-=sizeof(uint8_t))
					*((uint8_t*) va) = *((uint8_t*) (va-displacement));

				kprintf("Remap %u pages from physical start address 0x%zx linear to 0x%zx\n", npages, phyaddr, viraddr);
				ret = page_map(viraddr, phyaddr, npages, PG_GLOBAL|PG_RW);
				if (ret)
					goto failed;
			}
			break;
		case ELF_PT_GNU_STACK:	// Indicates stack executability => nothing todo
			break;
		default:
			kprintf("Unknown type %d\n", prog_header->type);
		}
	}

	kprintf("Entry point: 0x%zx\n", header->entry);
	// jump to the HermitCore app
	asm volatile ("jmp *%0" :: "r"(header->entry) : "memory");

	// we should never reach this point
	while(1) { HALT; }

failed:
	kputs("Upps, kernel panic!\n");
	while(1) { HALT; }

invalid:
	kprintf("Invalid executable!\n");
	kprintf("magic number 0x%x\n", (uint32_t) header->ident.magic);
	kprintf("header type 0x%x\n", (uint32_t) header->type);
	kprintf("machine type 0x%x\n", (uint32_t) header->machine);
	kprintf("elf ident class 0x%x\n", (uint32_t) header->ident._class);
	kprintf("elf identdata 0x%x\n", header->ident.data);
	kprintf("program entry point 0x%lx\n", (size_t) header->entry);
	while(1) { HALT; }
}
