/*
 * GINGE - GINGE Is Not Gp2x Emulator
 * (C) notaz, 2010-2011
 *
 * This work is licensed under the MAME license, see COPYING file for details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>

#include "header.h"
#include "realfuncs.h"
#include "syscalls.h"

char *bin_path;
char **g_argv;

#define CHECK_(val, fail_operator, expect, err_msg) \
  if (val fail_operator expect) { \
    fprintf(stderr, err_msg ", exiting (%d)\n", (int)(long)val); \
    return 1; \
  }

#define CHECK_EQ(val, expect, err_msg) \
  CHECK_(val, !=, expect, err_msg)

#define CHECK_NE(val, expect, err_msg) \
  CHECK_(val, ==, expect, err_msg)

#define HDR_CHECK_EQ(field, expect, err_msg) \
  CHECK_(hdr.field, !=, expect, err_msg)

#define HDR_CHECK_NE(field, expect, err_msg) \
  CHECK_(hdr.field, ==, expect, err_msg)

#define FAIL_PERROR(msg) { \
  perror(msg); \
  return 1; \
}

typedef struct {
  unsigned long start;
  unsigned long end;
} maps_range;

static int is_range_used(maps_range *maps, int map_cnt, unsigned long start, unsigned long end)
{
  int i;
  for (i = 0; i < map_cnt; i++) {
    if (maps[i].end <= start)
      continue;
    if (end <= maps[i].start)
      continue;

    return i + 1;
  }

  return 0;
}

extern char **environ;

int main(int argc, char *argv[])
{
  void *lowest_segments[2] = { NULL, NULL };
  Elf32_Ehdr hdr;
  Elf32_Phdr *phdr;
  FILE *fi;
  maps_range maps[16];
  int map_cnt;
  int i, ret, envc, sfp;
  long *stack_frame;
  struct stat st;
  char buf[64];
  long lret;

  if (argc < 2) {
    fprintf(stderr, "usage: %s <program> [args]\n", argv[0]);
    return 1;
  }

  g_argv = argv;

  lret = g_personality(-1);
  if (g_syscall_error(lret) != -1) {
    lret |= 0x0240000; // ADDR_COMPAT_LAYOUT | ADDR_NO_RANDOMIZE
    g_personality(lret);
  }

  fi = fopen("/proc/self/maps", "r");
  CHECK_NE(fi, NULL, "fopen maps");

  for (i = 0; i < ARRAY_SIZE(maps); i++) {
    ret = fscanf(fi, "%lx-%lx %*s %*s %*s %*s %*s\n", &maps[i].start, &maps[i].end);
    if (ret <= 0)
      break;
    CHECK_EQ(ret, 2, "maps parse error");
  }
  fclose(fi);
  map_cnt = i;
  CHECK_NE(map_cnt, 0, "no maps");
  CHECK_NE(map_cnt, ARRAY_SIZE(maps), "too many maps");

  fi = fopen(argv[1], "rb");
  if (fi == NULL)
    FAIL_PERROR("fopen");

  if (fread(&hdr, 1, sizeof(hdr), fi) != sizeof(hdr))
    FAIL_PERROR("too small or");

  if (memcmp(hdr.e_ident, ELFMAG "\x01\x01", SELFMAG + 2) != 0) {
    fprintf(stderr, "not 32bit LE ELF?\n");
    return 1;
  }

  HDR_CHECK_EQ(e_type, ET_EXEC, "not executable");
  HDR_CHECK_EQ(e_machine, EM_ARM, "not ARM");
  HDR_CHECK_EQ(e_phentsize, sizeof(Elf32_Phdr), "bad PH entry size");
  HDR_CHECK_NE(e_phnum, 0, "no PH entries");

  phdr = malloc(hdr.e_phnum * hdr.e_phentsize);
  CHECK_NE(phdr, NULL, "OOM");

  if (fread(phdr, hdr.e_phentsize, hdr.e_phnum, fi) != hdr.e_phnum)
    FAIL_PERROR("too small or");

  for (i = 0; i < hdr.e_phnum; i++) {
    Elf32_Addr end_addr = phdr[i].p_vaddr + phdr[i].p_memsz;
    Elf32_Addr align;
    void *ptr, *map_ptr;

    if (phdr[i].p_type == PT_NOTE)
      continue;
    if (phdr[i].p_type != PT_LOAD) {
      fprintf(stderr, "skipping section %d\n", phdr[i].p_type);
      continue;
    }

    ret = is_range_used(maps, map_cnt, phdr[i].p_vaddr, end_addr);
    if (ret) {
      fprintf(stderr, "segment %d (%08x-%08x) hits %08lx-%08lx in maps\n",
        i, phdr[i].p_vaddr, end_addr, maps[ret - 1].start, maps[ret - 1].end);
      return 1;
    }

    log("load %d %08x-%08x from %08x\n", phdr[i].p_type,
      phdr[i].p_vaddr, end_addr, phdr[i].p_offset);

    align = phdr[i].p_vaddr & 0xfff;
    map_ptr = (void *)(phdr[i].p_vaddr - align);
    ptr = mmap(map_ptr, phdr[i].p_memsz + align, PROT_READ|PROT_WRITE|PROT_EXEC,
      MAP_FIXED|MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED || ptr != map_ptr)
      FAIL_PERROR("mmap");

    if (phdr[i].p_filesz > 0) {
      if (fseek(fi, phdr[i].p_offset, SEEK_SET) != 0)
        FAIL_PERROR("fseek");
      if (fread((char *)ptr + align, 1, phdr[i].p_filesz, fi) != phdr[i].p_filesz)
        FAIL_PERROR("too small or");

      if (phdr[i].p_flags & PF_X)
        do_patches((char *)ptr + align, phdr[i].p_filesz);
    }

    if (lowest_segments[0] == NULL || map_ptr < lowest_segments[0])
      lowest_segments[0] = map_ptr;
  }

  // build self bin path
  snprintf(buf, sizeof(buf), "/proc/self/fd/%d", fileno(fi));
  if (lstat(buf, &st) != 0)
    FAIL_PERROR("lstat bin_path");
  bin_path = malloc(st.st_size + 1);
  CHECK_NE(bin_path, NULL, "bin_path");
  ret = readlink(buf, bin_path, st.st_size);
  if (ret < 0)
    FAIL_PERROR("readlink");
  bin_path[ret] = 0;

  fclose(fi);

  emu_init(lowest_segments, 0);

  // generate stack frame: argc, argv[], NULL, env[], NULL
  for (envc = 0; environ[envc] != NULL; envc++)
    ;

  stack_frame = calloc(argc + envc + 3, sizeof(stack_frame[0]));
  if (stack_frame == NULL) {
    fprintf(stderr, "stack_frame OOM\n");
    return 1;
  }

  // update the environment
  setenv("_", bin_path, 1);

  sfp = 0;
  stack_frame[sfp++] = argc - 1;
  for (i = 1; i < argc; i++)
    stack_frame[sfp++] = (long)argv[i];
  stack_frame[sfp++] = 0;
  for (i = 0; i < envc; i++)
    stack_frame[sfp++] = (long)environ[i];
  stack_frame[sfp++] = 0;

  log("entering %08x, %d stack entries\n", hdr.e_entry, sfp);
  do_entry(hdr.e_entry, stack_frame, sfp, NULL);

  fprintf(stderr, "do_entry failed!\n");
  return 1;
}

// vim:shiftwidth=2:expandtab
