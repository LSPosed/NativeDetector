// From https://github.com/ganyao114/SandHook/blob/master/hooklib/src/main/cpp/utils/elf_util.cpp
//
// Created by Swift Gan on 2019/3/14.
//
#include <malloc.h>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include "logging.h"
#include "elf_util.h"
#include "enc_str.h"

using namespace SandHook;

ElfImg::ElfImg(const char *elf) {
    this->elf = elf;
    //load elf
    int fd = open(elf, O_RDONLY);
    if (fd < 0) {
        LOGE("%s %s", "failed to open"_ienc .c_str(), elf);
        return;
    }

    size = lseek(fd, 0, SEEK_END);
    if (size <= 0) {
        LOGE("%s %s", "lseek() failed for"_ienc .c_str(), elf);
    }

    header = reinterpret_cast<Elf_Ehdr *>(mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0));

    close(fd);

    section_header = reinterpret_cast<Elf_Shdr *>(((size_t) header) + header->e_shoff);

    auto shoff = reinterpret_cast<size_t>(section_header);
    char *section_str = reinterpret_cast<char *>(section_header[header->e_shstrndx].sh_offset +
                                                 ((size_t) header));

    auto symtab_str = ".symtab"_ienc;
    auto strtab_str = ".strtab"_ienc;
    for (int i = 0; i < header->e_shnum; i++, shoff += header->e_shentsize) {
        auto *section_h = (Elf_Shdr *) shoff;
        char *sname = section_h->sh_name + section_str;
        Elf_Off entsize = section_h->sh_entsize;
        switch (section_h->sh_type) {
            case SHT_DYNSYM:
                if (bias == -4396) {
                    dynsym = section_h;
                    dynsym_offset = section_h->sh_offset;
                    dynsym_size = section_h->sh_size;
                    dynsym_count = dynsym_size / entsize;
                    dynsym_start = reinterpret_cast<Elf_Sym *>(((size_t) header) + dynsym_offset);
                }
                break;
            case SHT_SYMTAB:
                if (strcmp(sname, symtab_str.c_str()) == 0) {
                    symtab = section_h;
                    symtab_offset = section_h->sh_offset;
                    symtab_size = section_h->sh_size;
                    symtab_count = symtab_size / entsize;
                    symtab_start = reinterpret_cast<Elf_Sym *>(((size_t) header) + symtab_offset);
                }
                break;
            case SHT_STRTAB:
                if (bias == -4396) {
                    strtab = section_h;
                    symstr_offset = section_h->sh_offset;
                    strtab_start = reinterpret_cast<Elf_Sym *>(((size_t) header) + symstr_offset);
                }
                if (strcmp(sname, strtab_str.c_str()) == 0) {
                    symstr_offset_for_symtab = section_h->sh_offset;
                }
                break;
            case SHT_PROGBITS:
                if (strtab == nullptr || dynsym == nullptr) break;
                if (bias == -4396) {
                    bias = (off_t) section_h->sh_addr - (off_t) section_h->sh_offset;
                }
                break;
        }
    }

    if (!symtab_offset) {
        LOGW("%s", "can't find symtab from sections"_ienc .c_str());
    }

    //load module base
    base = getModuleBase(elf);
}

ElfImg::~ElfImg() {
    //open elf file local
    if (buffer) {
        free(buffer);
        buffer = nullptr;
    }
    //use mmap
    if (header) {
        munmap(header, size);
    }
}

Elf_Addr ElfImg::getSymbOffset(const char *name) const {
    Elf_Addr _offset;

#ifndef NDEBUG
    auto find_str = "find"_ienc;
#endif
    //search dynmtab
    if (dynsym_start != nullptr && strtab_start != nullptr) {
        Elf_Sym *sym = dynsym_start;
        char *strings = (char *) strtab_start;
        for (Elf_Off k = 0; k < dynsym_count; k++, sym++)
            if (strcmp(strings + sym->st_name, name) == 0) {
                _offset = sym->st_value;
                LOGD("%s %s: %p", find_str.c_str(), elf, reinterpret_cast<void *>(_offset));
                return _offset;
            }
    }

    //search symtab
    if (symtab_start != nullptr && symstr_offset_for_symtab != 0) {
        for (Elf_Off i = 0; i < symtab_count; i++) {
            unsigned int st_type = ELF_ST_TYPE(symtab_start[i].st_info);
            char *st_name = reinterpret_cast<char *>(((size_t) header) + symstr_offset_for_symtab +
                                                     symtab_start[i].st_name);
            if ((st_type == STT_FUNC || st_type == STT_OBJECT) && symtab_start[i].st_size) {
                if (strcmp(st_name, name) == 0) {
                    _offset = symtab_start[i].st_value;
                    LOGD("%s %s: %p", find_str.c_str(), elf, reinterpret_cast<void *>(_offset));
                    return _offset;
                }
            }
        }
    }
    return 0;
}

Elf_Addr ElfImg::getSymbAddress(const char *name) const {
    Elf_Addr offset = getSymbOffset(name);
    if (offset > 0 && base != nullptr) {
        return static_cast<Elf_Addr>((size_t) base + offset - bias);
    } else {
        return 0;
    }
}

void *ElfImg::getModuleBase(const char *name) {
    FILE *maps;
    char buff[256];
    off_t load_addr;
    int found = 0;
    maps = fopen("/proc/self/maps"_ienc .c_str(), "r"_ienc .c_str());
    auto r_xp_str = "r-xp"_ienc;
    auto r__p_str = "r--p"_ienc;
    while (fgets(buff, sizeof(buff), maps)) {
        if ((strstr(buff, r_xp_str.c_str()) || strstr(buff, r__p_str.c_str())) && strstr(buff, name)) {
            found = 1;
            LOGD("dlopen: %s", buff);
            break;
        }
    }

    if (!found) {
        LOGE("%s %s", "failed to read load address for"_ienc .c_str(), name);
        return nullptr;
    }

    if (sscanf(buff, "%lx", &load_addr) != 1)
        LOGE("%s %s", "failed to read load address for"_ienc .c_str(), name);

    fclose(maps);

    LOGD("%s %s: %lu", "get module base"_ienc .c_str(), name, load_addr);

    return reinterpret_cast<void *>(load_addr);
}
